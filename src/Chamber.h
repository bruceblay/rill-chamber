// Copyright (c) 2026 Bruce Blay
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cstdint>
#include "Ensemble.h"

// What a chamber ensemble agrees on beyond the clock: who is here, which piece,
// and when it starts. No radio or audio in here, so it runs on the host.
//
// Every device broadcasts, not just the conductor, so each one knows who else
// is present. A press of the front button on any device is a proposal: it
// carries a higher epoch than the last one, and the newest proposal wins
// everywhere, with the lower id breaking a tie. That is the same rule the
// family already uses for sharing a key, and it means either device in your
// hands can start or stop the piece.
namespace chamber {

constexpr unsigned maxMembers = 4;

struct Control {
  uint32_t epoch;
  uint32_t from;      // id of the device that proposed this
  int64_t start;      // shared microseconds at which pulse 0 of the score sounds
  uint32_t members[maxMembers];  // who plays, lowest id first: parts are dealt by rank
  uint8_t piece;
  uint8_t running;
  uint8_t count;      // how many of `members` are filled
  uint8_t spare;
} __attribute__((packed));

// One broadcast: the clock (read only by followers of its conductor) and the
// ensemble's state. Well under ESP-NOW's 250-byte limit.
struct Packet {
  ensemble::Packet clock;
  Control control;
  uint32_t sender;  // who sent this, for the roster
} __attribute__((packed));
static_assert(sizeof(Packet) <= 250, "ESP-NOW payload");

inline bool newer(const Control& a, const Control& b) {
  if (a.epoch != b.epoch) return a.epoch > b.epoch;
  return a.epoch != 0 && a.from < b.from;
}

// Who has been heard from lately, including this device.
class Roster {
 public:
  void heard(uint32_t id, int64_t now) {
    for (auto& entry : entries_) if (entry.id == id) { entry.at = now; return; }
    for (auto& entry : entries_) if (!entry.id) { entry = {id, now}; return; }
    auto oldest = std::min_element(entries_, entries_ + capacity,
                                   [](const Entry& a, const Entry& b) { return a.at < b.at; });
    *oldest = {id, now};
  }
  // Present devices, lowest id first, at most maxMembers. Anyone silent for
  // three seconds has gone.
  unsigned members(uint32_t self, int64_t now, uint32_t* out, int64_t patience = 3000000) const {
    uint32_t ids[capacity + 1];
    unsigned count = 0;
    ids[count++] = self;
    for (const auto& entry : entries_)
      if (entry.id && entry.id != self && now - entry.at < patience) ids[count++] = entry.id;
    std::sort(ids, ids + count);
    count = std::min(count, maxMembers);
    std::copy(ids, ids + count, out);
    return count;
  }

 private:
  static constexpr unsigned capacity = 8;
  struct Entry { uint32_t id; int64_t at; };
  Entry entries_[capacity]{};
};

class Session {
 public:
  void begin(uint32_t self) { self_ = self; control_ = Control{}; }
  const Control& control() const { return control_; }

  // This device's proposal, which becomes the ensemble's once others hear it.
  void propose(uint8_t piece, bool running, int64_t start, const uint32_t* members, unsigned count) {
    Control next{};
    next.epoch = control_.epoch + 1;
    next.from = self_;
    next.start = start;
    next.piece = piece;
    next.running = running ? 1 : 0;
    next.count = uint8_t(std::min(count, maxMembers));
    std::copy(members, members + next.count, next.members);
    control_ = next;
  }

  // Returns true if the ensemble's state changed.
  bool receive(const Control& incoming) {
    if (!newer(incoming, control_)) return false;
    control_ = incoming;
    return true;
  }

  // This device's rank among the players, or -1 if it is listening only.
  int rank() const {
    for (unsigned i = 0; i < control_.count && i < maxMembers; ++i)
      if (control_.members[i] == self_) return int(i);
    return -1;
  }

 private:
  uint32_t self_ = 0;
  Control control_{};
};

}  // namespace chamber
