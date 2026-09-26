// Copyright (c) 2026 Bruce Blay
// SPDX-License-Identifier: GPL-3.0-or-later
#include <cstdio>
#include <algorithm>
#include <cstdlib>
#include "Chamber.h"

static int failures = 0;
#define CHECK(condition) do { if (!(condition)) { ++failures; std::printf("FAIL line %d: %s\n", __LINE__, #condition); } } while (0)

int main() {
  // The roster: present devices, lowest id first, the silent ones dropped.
  chamber::Roster roster;
  roster.heard(30, 0);
  roster.heard(10, 0);
  uint32_t members[chamber::maxMembers]{};
  CHECK(roster.members(20, 1000000, members) == 3);
  CHECK(members[0] == 10 && members[1] == 20 && members[2] == 30);
  roster.heard(10, 3500000);
  CHECK(roster.members(20, 4000000, members) == 2);  // 30 went quiet
  CHECK(members[0] == 10 && members[1] == 20);
  CHECK(roster.members(20, 9000000, members) == 1 && members[0] == 20);  // alone

  // Proposals: the newest wins everywhere, whoever made it.
  chamber::Session a, b;
  a.begin(10);
  b.begin(20);
  uint32_t duo[] = {10, 20};
  b.propose(1, true, 5000000, duo, 2);
  CHECK(a.receive(b.control()));
  CHECK(a.control().piece == 1 && a.control().running && a.control().start == 5000000);
  CHECK(a.rank() == 0 && b.rank() == 1);
  CHECK(!a.receive(b.control()));  // the same proposal twice changes nothing
  a.propose(1, false, 0, duo, 2);   // either device can stop it
  CHECK(b.receive(a.control()));
  CHECK(!b.control().running);

  // Two proposals at once: equal epochs, and the lower id wins on both sides.
  chamber::Session c, d;
  c.begin(10);
  d.begin(20);
  c.propose(0, true, 1, duo, 2);
  d.propose(1, true, 2, duo, 2);
  chamber::Control fromC = c.control(), fromD = d.control();
  c.receive(fromD);
  d.receive(fromC);
  CHECK(c.control().piece == 0 && d.control().piece == 0);

  // A device not dealt in listens.
  uint32_t others[] = {10, 30};
  chamber::Session e;
  e.begin(20);
  e.receive([&] { chamber::Session f; f.begin(10); f.propose(0, true, 0, others, 2); return f.control(); }());
  CHECK(e.rank() == -1);

  // The shared timeline runs on without a jump as beats go by.
  ensemble::Clock clock;
  clock.begin(1, 0, 240);
  int64_t before = clock.sharedMicros(249999);
  CHECK(clock.due(250000));
  int64_t after = clock.sharedMicros(250001);
  CHECK(after - before == 2);

  // A follower that has not yet ticked past a beat hears a packet naming the
  // conductor's next one. Its shared time must not move: that is the moment a
  // truncating division once counted it a whole beat ahead.
  {
    ensemble::Clock lead, follow;
    const int64_t period = 250000;
    lead.begin(1, 0, 240);
    follow.begin(2, 0, 240);
    // Line them up, then run both well into the performance.
    int64_t t = 0;
    for (; t < 10000000; t += 1000) {
      lead.settle(t);
      follow.settle(t);
      while (lead.due(t)) {}
      if (t % 250000 == 0) follow.receive(lead.outgoing(t), t);
      while (follow.due(t)) {}
    }
    CHECK(lead.conducting() && !follow.conducting());
    // The lead ticks past a beat and sends; the follower receives just before
    // its own tick for the same beat.
    int64_t beat = lead.nextBeat();
    while (lead.due(beat)) {}
    ensemble::Packet packet = lead.outgoing(beat + 10);
    int64_t before = follow.sharedMicros(beat + 20);
    follow.receive(packet, beat + 20);
    int64_t after = follow.sharedMicros(beat + 20);
    CHECK(std::llabs(after - before) < period / 10);
    CHECK(std::llabs(follow.sharedMicros(beat + 20) - lead.sharedMicros(beat + 20)) < period / 10);
  }

  // The leader restarts mid-piece. It must come back on the timeline everyone
  // is already on, or the piece's start time points at a timeline that no
  // longer exists and every device falls silent.
  {
    ensemble::Clock lead, follow;
    lead.begin(1, 0, 240);
    follow.begin(2, 0, 240);
    int64_t worstJump = 0, previous = 0;
    bool restarted = false;
    for (int64_t t = 0; t < 20000000; t += 1000) {
      if (t == 8000000) { lead = ensemble::Clock(); lead.begin(1, t, 240); restarted = true; }
      lead.settle(t);
      follow.settle(t);
      follow.checkTimeout(t);
      lead.checkTimeout(t);
      while (lead.due(t)) {}
      while (follow.due(t)) {}
      // Everyone broadcasts four times a second, as the firmware does.
      if (t % 250000 == 0) follow.receive(lead.outgoing(t), t);
      if (t % 250000 == 125000) lead.receive(follow.outgoing(t), t);
      int64_t shared = follow.sharedMicros(t);
      if (t > 3000000) worstJump = std::max<int64_t>(worstJump, std::llabs(shared - previous - 1000));
      previous = shared;
    }
    CHECK(restarted && lead.conducting() && !follow.conducting());
    CHECK(worstJump < 20000);  // the follower's time never jumps
    CHECK(std::llabs(lead.sharedMicros(20000000) - follow.sharedMicros(20000000)) < 5000);
    std::printf("restart: follower's worst jump %lld us, clocks %lld us apart\n", (long long)worstJump,
                (long long)std::llabs(lead.sharedMicros(20000000) - follow.sharedMicros(20000000)));
  }

  if (failures) { std::printf("%d failures\n", failures); return 1; }
  std::printf("chamber: roster, proposals and shared time behave\n");
  return 0;
}
