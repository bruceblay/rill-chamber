// Copyright (c) 2026 Bruce Blay
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include "Pieces.h"

// The lab's score engine, ported line for line from rill-sound's
// phase-piece.js so a part plays here exactly as it does in the browser. Time
// is counted in pulses from the start of the score; nothing here touches audio
// or the radio, so the whole thing runs on the host against note times the
// JavaScript engine produced (tests/score_vectors.h).
namespace score {

inline const Piece& piece(unsigned index) { return pieces[index % pieceCount]; }
inline const Part& part(const Piece& p, unsigned index) { return parts[p.part + index]; }

// A piece's length in pulses: its longest part, whether a list of notes or a
// run of stages.
inline double piecePulses(const Piece& p) {
  double longest = 0;
  for (unsigned i = 0; i < p.partCount; ++i) {
    const Part& pt = part(p, i);
    if (pt.noteCount) {
      for (uint32_t n = 0; n < pt.noteCount; ++n)
        longest = std::max(longest, double(noteEvents[pt.note + n].start + noteEvents[pt.note + n].length));
    } else {
      double cycles = 0;
      for (unsigned s = 0; s < pt.stageCount; ++s) cycles += stages[pt.stage + s].cycles;
      longest = std::max(longest, cycles * p.cycle);
    }
  }
  return longest;
}

// Pieces are browsed a collection at a time; collections are contiguous in the
// table. Next within this collection (wrapping), and the first of the next.
inline bool sameCollection(unsigned a, unsigned b) { return !std::strcmp(piece(a).collection, piece(b).collection); }
inline unsigned collectionStart(unsigned index) {
  index %= pieceCount;
  while (index > 0 && sameCollection(index - 1, index)) --index;
  return index;
}
inline unsigned collectionSize(unsigned index) {
  unsigned start = collectionStart(index), end = start;
  while (end < pieceCount && sameCollection(start, end)) ++end;
  return end - start;
}
inline unsigned nextInCollection(unsigned index) {
  unsigned start = collectionStart(index);
  return start + (index % pieceCount - start + 1) % collectionSize(index);
}
inline unsigned nextCollection(unsigned index) {
  unsigned start = collectionStart(index);
  return (start + collectionSize(index)) % pieceCount;
}

// Where a part is at `position`, measured in cycles of the score. Rests keep
// the offset of whatever came before, so nothing jumps while silent. A stage
// with a rate moves its offset by (rate - 1) steps per step.
struct Where {
  const Stage* stage;
  unsigned index;
  double progress, phase;
  bool done;
};

inline Where stageAt(const Part& part, double position, unsigned steps) {
  double start = 0, phase = 0;
  for (unsigned i = 0; i < part.stageCount; ++i) {
    const Stage& stage = stages[part.stage + i];
    if (position < start + stage.cycles) {
      double progress = (position - start) / stage.cycles;
      if (stage.hasFrom) phase = stage.from + (stage.to - stage.from) * progress;
      if (stage.hasRate) phase += (stage.rate - 1) * (position - start) * steps;
      return {&stage, i, progress, phase, false};
    }
    if (stage.hasFrom) phase = stage.to;
    if (stage.hasRate) phase += (stage.rate - 1) * stage.cycles * steps;
    start += stage.cycles;
  }
  return {nullptr, part.stageCount, 1, phase, true};
}

// How fast a stage's offset changes, in steps per step. A note due in x steps
// satisfies x = gap - slope * x, so the exact wait is gap / (1 + slope).
inline double phaseSlope(const Stage& stage, unsigned steps) {
  double slope = stage.hasRate ? stage.rate - 1 : 0;
  if (stage.hasFrom && stage.cycles > 0) slope += (stage.to - stage.from) / (stage.cycles * steps);
  return slope;
}

// Whether a step of the figure sounds: all its notes while playing, the first
// few in build order during a build, fewer and fewer during a reduction.
inline bool audible(const Pattern& pattern, const Where& where, unsigned step) {
  if (!where.stage || where.stage->mode == Mode::Rest) return false;
  unsigned count = pattern.orderLength;
  if (where.stage->mode == Mode::Build)
    count = std::min<unsigned>(count, 1 + unsigned(std::floor(where.progress * count)));
  if (where.stage->mode == Mode::Reduce) {
    int left = int(count) - 1 - int(std::floor(where.progress * count));
    count = unsigned(std::max(0, left));
  }
  for (unsigned i = 0; i < count; ++i)
    if (orders[pattern.order + i] == step) return true;
  return false;
}

inline int64_t wrap(int64_t n, int64_t length) { return ((n % length) + length) % length; }
// JavaScript's Math.round, which rounds halves up rather than away from zero.
inline int64_t jsRound(double x) { return int64_t(std::floor(x + 0.5)); }

// One part being played. `pulse` runs the lab's loop for one pulse and hands
// every note due within it to `emit(time, midi, length)`, time in pulses and
// length in pulses (zero for struck process parts, whose notes ring freely).
// A composed part plays its notes list instead, as the lab's Bach engine does:
// each note that starts within the pulse, at its place in the pulse.
class PartPlayer {
 public:
  void reset(uint32_t seed) {
    hasNext_ = false;
    next_ = 0;
    stageIndex_ = ~0u;
    stageNote_ = 0;
    slip_ = 0;
    step_ = 0;
    rng_ = seed ? seed : 1;
    noteIndex_ = 0;
    where_ = {nullptr, 0, 0, 0, false};
  }

  // Returns false once the part has finished.
  template <class Emit>
  bool pulse(const Piece& piece, const Part& part, int64_t local, Emit&& emit) {
    if (local < 0) return true;
    if (part.noteCount) {
      while (noteIndex_ < part.noteCount && noteEvents[part.note + noteIndex_].start < float(local + 1)) {
        const NoteEvent& n = noteEvents[part.note + noteIndex_++];
        if (n.start < float(local)) continue;  // joined too late for this one
        step_ = int64_t(noteIndex_ - 1);
        emit(double(n.start), int(n.midi), double(n.length));
      }
      const NoteEvent& last = noteEvents[part.note + part.noteCount - 1];
      bool playing = noteIndex_ < part.noteCount || double(local) < double(last.start + last.length);
      where_ = {nullptr, 0, 0, 0, !playing};
      return playing;
    }
    const unsigned steps = piece.cycle;
    Where where = stageAt(part, double(local) / steps, steps);
    where_ = where;
    if (where.done) return false;
    const Stage& stage = *where.stage;
    const Pattern& pattern = patterns[stage.pattern];
    const int64_t length = pattern.length;
    if (!hasNext_) { next_ = local + jsRound(where.phase); hasNext_ = true; }
    if (where.index != stageIndex_) { stageIndex_ = where.index; stageNote_ = next_; }
    const double speed = 1 + phaseSlope(stage, steps);
    for (;;) {
      double due = double(local) + (double(next_ - local) - where.phase) / speed;
      if (due >= double(local) + 1 - 1e-9) break;
      // Once lost, a part stays lost: the slip carries into every later note.
      if (stage.slips > 0 && stage.mode != Mode::Rest && unit() < stage.slips) slip_ += unit() < 0.5 ? 1 : -1;
      int64_t position = stage.restart ? next_ - stageNote_ + slip_ : next_;
      int64_t step = wrap(position, length);
      int note = stage.once && (position < 0 || position >= length) ? -1 : notes[pattern.notes + step];
      ++next_;
      if (due < double(local) - 0.25 || note < 0 || !audible(pattern, where, unsigned(step))) continue;
      step_ = stage.once ? position : step;
      emit(std::max(double(local), due), note, stage.hold);
    }
    return true;
  }

  const Where& where() const { return where_; }
  int64_t step() const { return step_; }
  int slip() const { return slip_; }

 private:
  double unit() {
    rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5;
    return double(rng_ >> 8) / 16777216.0;
  }
  bool hasNext_ = false;
  int64_t next_ = 0, stageNote_ = 0, step_ = 0;
  unsigned stageIndex_ = ~0u;
  uint32_t noteIndex_ = 0;
  int slip_ = 0;
  uint32_t rng_ = 1;
  Where where_{nullptr, 0, 0, 0, false};
};

// Which parts a device plays: parts are dealt out by rank, so with fewer
// devices than parts each device takes several, and one device alone plays
// the whole piece.
inline bool plays(unsigned partIndex, unsigned rank, unsigned members) {
  return members > 0 && partIndex % members == rank;
}

}  // namespace score
