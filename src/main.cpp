// Copyright (c) 2026 Bruce Blay
// SPDX-License-Identifier: GPL-3.0-or-later
#include <M5Unified.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <atomic>
#include <esp_system.h>
#include "Chamber.h"
#include "Player.h"
#include "Screen.h"

// Rill Chamber: process pieces for several Stick S3s, one part each, from the
// Rill Sound lab. Every device runs this same firmware. The lowest id keeps the
// clock, every device broadcasts who it is, and a press on any device proposes
// the next state for all of them. Parts are dealt by rank, so one device alone
// plays a whole piece and two share it.
//
// Front button: play or stop, for everyone. Side button: next piece in the
// collection while stopped (hold for the next collection), volume while playing.

static ensemble::Clock clock_;
static chamber::Roster roster;
static chamber::Session session;
static player::Engine engine;
static M5Canvas canvas(&M5.Display);
static uint32_t self = 0;
static uint8_t broadcast[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
static uint8_t volume = 255;  // as loud as the speaker goes; the side button steps it down
// A nearly flat battery cannot supply the speaker's peaks at full volume: the
// voltage collapses and the device resets mid-piece. Below these voltages the
// volume is capped, quieter as the battery runs down, whatever was chosen.
static uint8_t volumeCap = 255;
static int batteryMillivolts = 0;
static uint8_t capFor(int millivolts) {
  if (millivolts <= 0) return 255;       // no reading: assume a healthy supply
  if (millivolts < 3450) return 140;
  if (millivolts < 3650) return 190;
  return 255;
}
static void applyVolume() { M5.Speaker.setVolume(std::min(volume, volumeCap)); }

// Received packets are copied out on the WiFi task and handled in the loop.
static volatile bool pending = false;
static chamber::Packet inbox{};
static volatile int64_t inboxAt = 0;
static std::atomic<uint32_t> dropped{0};

// The shared timeline as last read by the loop: shared microseconds at a local
// time. The audio task extrapolates from it, under this lock.
static portMUX_TYPE timingLock = portMUX_INITIALIZER_UNLOCKED;
static int64_t timingLocal = 0, timingShared = 0;
// The ensemble state the audio task should be playing, and a count that moves
// on every change so it knows to reconfigure.
struct Plan { uint8_t piece; bool running; int64_t start; int rank; unsigned members; uint32_t seed; };
static Plan plan{};
static std::atomic<uint32_t> planGeneration{0};
static std::atomic<uint32_t> worstRenderUs{0}, queueErrors{0}, worstGapUs{0};
// The longest pass through the loop: a stall here freezes the radio and screen.
static uint32_t worstLoopUs = 0;

void onReceive(const uint8_t*, const uint8_t* data, int length) {
  int64_t at = esp_timer_get_time();
  if (length != int(sizeof(chamber::Packet))) return;
  if (pending) { ++dropped; return; }
  std::memcpy(const_cast<chamber::Packet*>(&inbox), data, sizeof(chamber::Packet));
  inboxAt = at;
  pending = true;
}

static uint32_t deviceId() {
  uint8_t mac[6]{};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  return (uint32_t(mac[2]) << 24) | (uint32_t(mac[3]) << 16) | (uint32_t(mac[4]) << 8) | mac[5];
}

static bool startRadio() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  // A fixed channel: there is no access point to agree one with.
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init() != ESP_OK) return false;
  esp_now_register_recv_cb(onReceive);
  esp_now_peer_info_t peer{};
  std::memcpy(peer.peer_addr, broadcast, 6);
  peer.channel = 1;
  peer.encrypt = false;
  return esp_now_add_peer(&peer) == ESP_OK;
}

static int64_t sharedNow(int64_t now) {
  portENTER_CRITICAL(&timingLock);
  int64_t shared = timingShared + (now - timingLocal);
  portEXIT_CRITICAL(&timingLock);
  return shared;
}

// Hand the ensemble's current state to the audio task.
static void applyControl() {
  const chamber::Control& control = session.control();
  portENTER_CRITICAL(&timingLock);
  plan = {control.piece, control.running != 0, control.start, session.rank(), control.count, control.epoch ^ control.from};
  portEXIT_CRITICAL(&timingLock);
  ++planGeneration;
}

void audioTask(void*) {
  static int16_t buffers[3][512];
  unsigned index = 0;
  uint32_t seen = 0;
  Plan current{};
  bool sounding = false;
  for (;;) {
    uint32_t generation = planGeneration.load();
    if (generation != seen) {
      seen = generation;
      portENTER_CRITICAL(&timingLock);
      current = plan;
      portEXIT_CRITICAL(&timingLock);
      sounding = false;
    }
    // A start more than ten seconds away cannot be one a device proposed (they
    // start two and a half seconds out): it belongs to a timeline that has
    // since moved, so treat the piece as stopped rather than wait in silence.
    const bool stale = current.running && current.start - sharedNow(esp_timer_get_time()) > 10000000;
    const bool wanted = current.running && !stale;
    if (wanted != sounding) {
      if (wanted) engine.configure(current.piece, current.rank, current.members, current.seed);
      else engine.silence();
      sounding = wanted;
    }
    double position = -1;
    if (sounding) {
      const score::Piece& piece = score::piece(current.piece);
      position = double(sharedNow(esp_timer_get_time()) - current.start) / double(piece.periodMicros);
    }
    uint32_t started = micros();
    // The longest wait between blocks: a stall here is a gap in the sound.
    static uint32_t lastStart = 0;
    if (lastStart && started - lastStart > worstGapUs) worstGapUs = started - lastStart;
    lastStart = started;
    engine.render(buffers[index], 512, position);
    uint32_t elapsed = micros() - started;
    if (elapsed > worstRenderUs) worstRenderUs = elapsed;
    while (!M5.Speaker.playRaw(buffers[index], 512, player::rate, false, 1, 0)) {
      ++queueErrors;
      vTaskDelay(1);
    }
    index = (index + 1) % 3;
  }
}

static void send(int64_t now) {
  chamber::Packet packet{};
  packet.clock = clock_.outgoing(now);
  packet.control = session.control();
  packet.sender = self;
  esp_now_send(broadcast, reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
}

static unsigned members(int64_t now, uint32_t* out) { return roster.members(self, now, out); }

// Why this device last started, so a restart mid-piece can be told apart:
// a crash, a watchdog, or the battery dipping under a loud passage.
static const char* resetName() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON: return "power-on";
    case ESP_RST_SW: return "software";
    case ESP_RST_PANIC: return "crash";
    case ESP_RST_INT_WDT: return "interrupt-watchdog";
    case ESP_RST_TASK_WDT: return "task-watchdog";
    case ESP_RST_WDT: return "watchdog";
    case ESP_RST_BROWNOUT: return "brownout";
    case ESP_RST_DEEPSLEEP: return "deep-sleep";
    default: return "other";
  }
}

void setup() {
  auto cfg = M5.config();
  cfg.internal_spk = true;
  cfg.internal_mic = false;
  cfg.internal_imu = false;
  M5.begin(cfg);
  Serial.begin(115200);
  // With no computer attached nothing reads the USB serial port, and a write
  // would wait for its timeout, stalling the loop that runs the radio and the
  // screen. Running on battery, that was the whole difference. Never wait:
  // what cannot be sent is dropped.
  Serial.setTxTimeoutMs(0);
  M5.BtnB.setHoldThresh(600);
  M5.Display.setRotation(0);
  M5.Display.setBrightness(110);
  canvas.setColorDepth(16);
  canvas.setPsram(true);
  canvas.createSprite(135, 240);
  self = deviceId();
  session.begin(self);
  bool radio = startRadio();
  clock_.begin(self, esp_timer_get_time(), 240);
  M5.Speaker.begin();
  batteryMillivolts = M5.Power.getBatteryVoltage();
  volumeCap = capFor(batteryMillivolts);
  applyVolume();
  applyControl();
  xTaskCreatePinnedToCore(audioTask, "chamber-audio", 12288, nullptr, 3, nullptr, 1);
  Serial.printf("rill-chamber id=%08lx radio=%s pieces=%u reset=%s\n", (unsigned long)self, radio ? "up" : "FAILED",
                score::pieceCount, resetName());
}

void loop() {
  M5.update();
  const int64_t now = esp_timer_get_time();
  static int64_t lastLoop = 0;
  if (lastLoop && now - lastLoop > worstLoopUs) worstLoopUs = uint32_t(now - lastLoop);
  lastLoop = now;

  if (pending) {
    chamber::Packet packet = const_cast<chamber::Packet&>(inbox);
    int64_t at = inboxAt;
    pending = false;
    roster.heard(packet.sender, at);
    clock_.receive(packet.clock, at);
    if (session.receive(packet.control)) applyControl();
  }
  clock_.settle(now);
  clock_.checkTimeout(now);
  while (clock_.due(now)) {}
  portENTER_CRITICAL(&timingLock);
  timingLocal = now;
  timingShared = clock_.sharedMicros(now);
  portEXIT_CRITICAL(&timingLock);

  // Everyone broadcasts, four times a second, so everyone knows who is here.
  static int64_t lastSend = 0;
  if (now - lastSend > 250000) { lastSend = now; send(now); }

  const chamber::Control& control = session.control();
  if (M5.BtnA.wasClicked()) {
    uint32_t ids[chamber::maxMembers];
    unsigned count = members(now, ids);
    // Start a couple of seconds out, so every device has the proposal first.
    if (control.running) session.propose(control.piece, false, 0, ids, count);
    else session.propose(control.piece, true, sharedNow(now) + 2500000, ids, count);
    applyControl();
    send(now);
  }
  const bool nextPiece = M5.BtnB.wasClicked(), nextSet = M5.BtnB.wasHold();
  if ((nextPiece || nextSet) && control.running) {
    if (nextPiece) {
      volume = volume == 255 ? 170 : volume == 170 ? 110 : 255;
      applyVolume();
    }
  } else if (nextPiece || nextSet) {
    uint32_t ids[chamber::maxMembers];
    unsigned count = members(now, ids);
    unsigned next = nextSet ? score::nextCollection(control.piece) : score::nextInCollection(control.piece);
    session.propose(uint8_t(next), false, 0, ids, count);
    applyControl();
    send(now);
  }

  // Battery, once a second. A reading sags under load, so the cap only lifts
  // again once the voltage has recovered well past the step.
  static int64_t lastBattery = 0;
  if (now - lastBattery > 1000000) {
    lastBattery = now;
    batteryMillivolts = M5.Power.getBatteryVoltage();
    uint8_t cap = capFor(batteryMillivolts);
    if (cap < volumeCap || (cap > volumeCap && capFor(batteryMillivolts - 80) == cap)) {
      volumeCap = cap;
      applyVolume();
    }
  }

  static int64_t lastDraw = 0;
  if (now - lastDraw > 66000) {
    lastDraw = now;
    if (control.running) {
      player::View views[player::maxParts];
      unsigned count = engine.views(views);
      const score::Piece& piece = score::piece(control.piece);
      double pulses = double(sharedNow(now) - control.start) / double(piece.periodMicros);
      screen::playing(canvas, piece, views, count, session.rank() == 0, pulses, clock_.offsetJitter());
    } else {
      uint32_t ids[chamber::maxMembers];
      unsigned count = members(now, ids);
      int rank = 0;
      for (unsigned i = 0; i < count; ++i) if (ids[i] == self) rank = int(i);
      screen::menu(canvas, control.piece, count, rank, M5.Power.getBatteryLevel(), volumeCap < 255);
    }
    canvas.pushSprite(0, 0);
  }

  static int64_t lastReport = 0;
  if (now - lastReport > 5000000) {
    lastReport = now;
    uint32_t ids[chamber::maxMembers];
    Serial.printf("%s devices=%u piece=%u running=%u rank=%d jitter=%lldus render=%luus queue=%lu dropped=%lu "
                  "notes=%lu steals=%lu ahead=%lu back=%lu worst=%.2f late=%lu voices=%u gap=%luus "
                  "joins=%lu takeovers=%lu heard=%lu missed=%lu level=%.3f heap=%u reset=%s up=%llds loop=%luus "
                  "battery=%dmV%s volume=%u\n",
                  clock_.conducting() ? "lead" : "follow", members(now, ids), control.piece, control.running,
                  session.rank(), (long long)clock_.offsetJitter(), (unsigned long)worstRenderUs.load(),
                  (unsigned long)queueErrors.load(), (unsigned long)dropped.load(),
                  (unsigned long)engine.notesStarted(), (unsigned long)engine.steals(), (unsigned long)engine.snapsAhead(),
                  (unsigned long)engine.snapsBack(), engine.worstError(), (unsigned long)engine.lateNotes(),
                  engine.peakVoices(), (unsigned long)worstGapUs.load(), (unsigned long)clock_.joins(),
                  (unsigned long)clock_.takeovers(), (unsigned long)clock_.received(), (unsigned long)clock_.missed(),
                  engine.level(), ESP.getFreeHeap(), resetName(), (long long)(now / 1000000), (unsigned long)worstLoopUs,
                  int(M5.Power.getBatteryVoltage()), M5.Power.isCharging() == m5::Power_Class::is_charging ? "+" : "",
                  unsigned(std::min(volume, volumeCap)));
  }
  delay(2);
}
