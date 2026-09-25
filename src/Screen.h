// Copyright (c) 2026 Bruce Blay
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <M5Unified.h>
#include <cmath>
#include <cstring>
#include "Player.h"

// The screen, drawn as the lab's device screens are: the part's name, what it
// is doing, its offset from the leader in large type, and its figure as bars
// lined up under the leader's beat, so two devices side by side show their
// offset at a glance. Portrait, 135 by 240, into a sprite pushed once a frame.
namespace screen {

inline uint16_t rgb(uint32_t hex) { return M5.Display.color565(hex >> 16, (hex >> 8) & 255, hex & 255); }
inline uint16_t blend(uint32_t a, uint32_t b, float t) {
  auto mix = [&](int shift) { return int(((a >> shift) & 255) * (1 - t) + ((b >> shift) & 255) * t); };
  return M5.Display.color565(mix(16), mix(8), mix(0));
}
constexpr uint32_t background = 0x0b0e0c, ink = 0xe9eee0, green = 0xb8d98a, gold = 0xe4c77a, dim = 0x3a3f38;

inline void upper(char* out, const char* in, size_t size) {
  size_t i = 0;
  for (; in[i] && i + 1 < size; ++i) out[i] = char(std::toupper(unsigned(in[i])));
  out[i] = 0;
}

// "Piano 1" and "Piano 2" together read as "PIANO 1+2".
inline void partNames(const score::Piece& piece, const player::View* views, unsigned count, char* out, size_t size) {
  out[0] = 0;
  if (!count) { std::snprintf(out, size, "LISTENING"); return; }
  char first[32];
  upper(first, score::part(piece, views[0].part).name, sizeof first);
  std::snprintf(out, size, "%s", first);
  const char* space = std::strrchr(first, ' ');
  for (unsigned i = 1; i < count; ++i) {
    char name[32];
    upper(name, score::part(piece, views[i].part).name, sizeof name);
    const char* other = std::strrchr(name, ' ');
    size_t used = std::strlen(out);
    if (space && other && size_t(space - first) == size_t(other - name) && !std::strncmp(first, name, space - first))
      std::snprintf(out + used, size - used, "+%s", other + 1);
    else
      std::snprintf(out + used, size - used, "/%s", name);
  }
}

inline void centered(M5Canvas& c, const char* text, int y) {
  c.setTextDatum(top_center);
  c.drawString(text, 67, y);
  c.setTextDatum(top_left);
}

// Left-aligned, word-wrapped text; returns the y below it.
inline int wrapped(M5Canvas& c, const char* text, int x, int y, int width) {
  char line[48] = "";
  const char* word = text;
  while (*word) {
    const char* end = std::strchr(word, ' ');
    size_t length = end ? size_t(end - word) : std::strlen(word);
    char trial[48];
    std::snprintf(trial, sizeof trial, "%s%s%.*s", line, *line ? " " : "", int(length), word);
    if (*line && c.textWidth(trial) > width) {
      c.drawString(line, x, y);
      y += c.fontHeight() + 2;
      std::snprintf(line, sizeof line, "%.*s", int(length), word);
    } else {
      std::snprintf(line, sizeof line, "%s", trial);
    }
    word += length;
    while (*word == ' ') ++word;
  }
  if (*line) { c.drawString(line, x, y); y += c.fontHeight() + 2; }
  return y;
}

// Stopped: the piece, who is here, and what to press. Everything hangs off one
// left margin, as the playing screen does.
// Stopped: the piece, who is here, and what to press, each said once. The
// header carries the collection and the battery; under the title only what
// the header does not already say.
inline void menu(M5Canvas& c, unsigned pieceIndex, unsigned devices, int rank, int battery, bool lowBattery) {
  const score::Piece& piece = score::piece(pieceIndex);
  constexpr int left = 10, width = 115;
  c.fillScreen(rgb(background));
  c.setTextDatum(top_left);
  char text[48];

  c.setFont(&fonts::Font2);
  c.setTextColor(rgb(gold));
  upper(text, piece.collection, sizeof text);
  c.drawString(text, left, 10);
  if (battery >= 0) {
    std::snprintf(text, sizeof text, lowBattery ? "LOW %d%%" : "%d%%", std::min(battery, 100));
    c.setTextColor(rgb(lowBattery ? 0xce7067 : 0x8a9082));
    c.setTextDatum(top_right);
    c.drawString(text, left + width, 10);
    c.setTextDatum(top_left);
  }
  c.drawFastHLine(left, 32, width, rgb(dim));

  c.setFont(&fonts::FreeSansBold12pt7b);
  c.setTextColor(rgb(ink));
  int y = wrapped(c, piece.title, left, 44, width);
  if (*piece.byline) {
    c.setFont(&fonts::Font2);
    c.setTextColor(rgb(0x9aa092));
    y = wrapped(c, piece.byline, left, y + 2, width);
  }

  // Who plays what, now: parts are dealt by rank among the devices present.
  y = std::max(y + 14, 118);
  c.setFont(&fonts::Font2);
  c.setTextColor(rgb(green));
  std::snprintf(text, sizeof text, "%u part%s, %u device%s", piece.partCount, piece.partCount == 1 ? "" : "s",
                devices, devices == 1 ? "" : "s");
  c.drawString(text, left, y);
  if (rank >= 0 && devices) {
    char names[48] = "You: ";
    unsigned count = 0;
    for (unsigned i = 0; i < piece.partCount; ++i)
      if (score::plays(i, unsigned(rank), devices)) {
        size_t used = std::strlen(names);
        if (count++ < 2) std::snprintf(names + used, sizeof names - used, "%s%s", count > 1 ? ", " : "", score::part(piece, i).name);
      }
    if (count > 2) std::snprintf(names + std::strlen(names), sizeof names - std::strlen(names), " +%u", count - 2);
    c.setTextColor(rgb(ink));
    wrapped(c, names, left, y + 20, width);
  }

  c.drawFastHLine(left, 200, width, rgb(dim));
  c.setFont(&fonts::Font0);
  c.setTextColor(rgb(0x8a9082));
  const unsigned start = score::collectionStart(pieceIndex);
  std::snprintf(text, sizeof text, "%u/%u  Hold B: next set", pieceIndex % score::pieceCount - start + 1,
                score::collectionSize(pieceIndex));
  c.drawString(text, left, 208);
  c.setTextColor(rgb(ink));
  c.drawString("A  play", left, 224);
  c.setTextDatum(top_right);
  c.drawString("B  next", left + width, 224);
  c.setTextDatum(top_left);
}

inline void signedText(char* out, size_t size, double value, bool fraction) {
  if (fraction) {
    double rounded = std::round(value * 10) / 10;
    if (std::fabs(rounded) < 0.05) std::snprintf(out, size, "0");
    else std::snprintf(out, size, "%+.1f", rounded);
  } else {
    long whole = std::lround(value);
    if (whole == 0) std::snprintf(out, size, "0");
    else std::snprintf(out, size, "%+ld", whole);
  }
}

inline const char* noteName(int midi, char* out, size_t size) {
  static const char* names[] = {"C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#", "A", "Bb", "B"};
  std::snprintf(out, size, "%s%d", names[midi % 12], midi / 12 - 1);
  return out;
}

// A composed part: the bar, the note sounding now in large type, and the next
// two bars of the part as a little piano roll, as the lab's Bach pages show.
inline void composed(M5Canvas& c, const score::Piece& piece, const score::Part& part, const player::View& view,
                     uint32_t glow, uint16_t back, const char* lead, double pulses) {
  const score::NoteEvent* notes = &score::noteEvents[part.note];
  const float at = float(pulses);
  const int bar = int(std::floor(at / piece.beatsPerBar)) + 1;
  char status[32], main[12] = "-";
  if (pulses < 0) {
    std::snprintf(status, sizeof status, "%sREADY", lead);
    std::snprintf(main, sizeof main, "%d", int(std::ceil(-pulses * piece.periodMicros / 1e6)));
  } else if (view.done) {
    std::snprintf(status, sizeof status, "%sDONE", lead);
    std::snprintf(main, sizeof main, "OK");
  } else {
    std::snprintf(status, sizeof status, "%sBAR %d", lead, bar);
    // The note sounding now: the latest one started, if it has not ended.
    for (int64_t i = std::min<int64_t>(view.step, part.noteCount - 1); i >= 0 && i > view.step - 8; --i)
      if (notes[i].start <= at && at < notes[i].start + notes[i].length) { noteName(notes[i].midi, main, sizeof main); break; }
  }
  c.setFont(&fonts::FreeMono9pt7b);
  c.setTextColor(rgb(glow), back);
  c.drawString(status, 8, 32);
  c.setFont(&fonts::FreeSansBold24pt7b);
  c.setTextColor(rgb(ink), back);
  centered(c, main, 84);

  int low = 127, high = 0;
  for (uint32_t i = 0; i < part.noteCount; ++i) { low = std::min<int>(low, notes[i].midi); high = std::max<int>(high, notes[i].midi); }
  const float window = piece.beatsPerBar * 2.0f, from = std::max(0.0f, at);
  c.fillRect(8, 199, 119, 1, rgb(dim));
  for (int64_t i = std::max<int64_t>(0, view.step - 8); i < int64_t(part.noteCount); ++i) {
    const score::NoteEvent& n = notes[i];
    if (n.start > from + window) break;
    if (n.start + n.length < from) continue;
    const float left = std::max(n.start, from), right = std::min(n.start + n.length, from + window);
    const int x = 8 + int((left - from) / window * 119), w = std::max(2, int((right - left) / window * 119) - 1);
    const int y = 195 - (high == low ? 10 : (n.midi - low) * 38 / (high - low));
    const bool sounding = n.start <= at && at < n.start + n.length;
    c.fillRect(x, y, w, 3, sounding ? rgb(0xffffff) : rgb(glow));
  }
}

// Playing: this device's first part in detail, and a flash for every note it
// strikes across all its parts.
inline void playing(M5Canvas& c, const score::Piece& piece, const player::View* views, unsigned count,
                    bool leader, double pulses, int64_t jitterMicros) {
  c.fillScreen(rgb(background));
  const uint32_t glow = leader ? gold : green;
  float flash = 0;
  for (unsigned i = 0; i < count; ++i) flash = std::max(flash, views[i].flash);
  if (flash > 0.02f) c.fillScreen(blend(background, glow, flash * 0.3f));
  const uint16_t back = blend(background, glow, flash * 0.3f);
  char names[48];
  partNames(piece, views, count, names, sizeof names);
  c.setFont(&fonts::FreeMonoBold9pt7b);
  c.setTextColor(rgb(ink), back);
  c.drawString(names, 8, 10);
  if (!count) return;
  const player::View& view = views[0];
  const score::Part& part = score::part(piece, view.part);
  if (part.noteCount) {
    composed(c, piece, part, view, glow, back, "", pulses);
  } else {
  const score::Stage* stage = view.active ? &score::stages[part.stage + view.stage] : nullptr;
  char status[32] = "", main[16] = "";
  // The leader is marked by its gold, not by a word.
  const char* lead = "";
  if (view.done) { std::snprintf(status, sizeof status, "%sDONE", lead); std::snprintf(main, sizeof main, "OK"); }
  else if (!stage || pulses < 0) { std::snprintf(status, sizeof status, "%sREADY", lead); std::snprintf(main, sizeof main, "%d", int(std::ceil(-pulses * piece.periodMicros / 1e6))); }
  else {
    const bool drifting = stage->hasFrom && stage->from != stage->to;
    const char* word = stage->word && *stage->word ? stage->word
      : stage->slips > 0 ? (view.slip ? "LOST" : "IN STEP")
      : drifting ? "MOVING"
      : stage->mode == score::Mode::Rest ? "REST" : stage->mode == score::Mode::Build ? "BUILD"
      : stage->mode == score::Mode::Reduce ? "REDUCE" : "PLAY";
    std::snprintf(status, sizeof status, "%s%s", lead, word);
    if (stage->mode == score::Mode::Rest) std::snprintf(main, sizeof main, "-");
    else if (stage->main && *stage->main) std::snprintf(main, sizeof main, "%s", stage->main);
    else if (part.main && *part.main) std::snprintf(main, sizeof main, "%s", part.main);
    else if (stage->slips > 0) signedText(main, sizeof main, view.slip, false);
    else signedText(main, sizeof main, view.phase, drifting);
  }
  c.setFont(&fonts::FreeMono9pt7b);
  c.setTextColor(rgb(glow), back);
  c.drawString(status, 8, 32);
  c.setFont(&fonts::FreeSansBold24pt7b);
  c.setTextColor(rgb(ink), back);
  centered(c, main, 84);

  // The figure, as bars. Short figures line up under the leader's beat at this
  // part's offset; long ones show a window of the notes coming next.
  if (stage) {
    const score::Pattern& pattern = score::patterns[stage->pattern];
    int low = 127, high = 0;
    for (unsigned i = 0; i < pattern.length; ++i) {
      int note = score::notes[pattern.notes + i];
      if (note >= 0) { low = std::min(low, note); high = std::max(high, note); }
    }
    const bool longFigure = pattern.length > 24 || stage->restart;
    const unsigned shown = longFigure ? std::min<unsigned>(16, pattern.length) : pattern.length;
    const float width = 119.0f / shown;
    const int64_t now = int64_t(std::floor(pulses));
    const int64_t offset = int64_t(std::lround(view.phase));
    for (unsigned slot = 0; slot < shown; ++slot) {
      int64_t at = longFigure ? view.step + slot : score::wrap(slot + offset, pattern.length);
      int note = at < 0 || at >= pattern.length ? -1 : score::notes[pattern.notes + at];
      int x = 8 + int(slot * width);
      int w = std::max(2, int(width) - 2);
      if (note < 0) { c.fillRect(x, 196, w, 2, rgb(dim)); continue; }
      int height = 8 + (high == low ? 10 : (note - low) * 20 / (high - low));
      bool current = longFigure ? slot == 0 : int64_t(slot) == score::wrap(now, pattern.length);
      c.fillRect(x, 198 - height, w, height, current ? rgb(0xffffff) : rgb(glow));
    }
  }
  }
  c.setFont(&fonts::Font0);
  c.setTextColor(rgb(0x8a9082), back);
  // Time through the piece and its length, as a player would want to know.
  (void)jitterMicros;
  const double seconds = std::max(0.0, pulses) * piece.periodMicros / 1e6;
  const double length = score::piecePulses(piece) * piece.periodMicros / 1e6;
  char bottom[32];
  std::snprintf(bottom, sizeof bottom, "%d:%02d / %d:%02d", int(seconds) / 60, int(seconds) % 60,
                int(length) / 60, int(length) % 60);
  c.drawString(bottom, 8, 214);
  c.fillRect(8 + int(119 * (1 - flash) / 2), 230, int(119 * flash), 2, rgb(glow));
}

}  // namespace screen
