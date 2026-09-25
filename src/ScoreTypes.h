// Copyright (c) 2026 Bruce Blay
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>

// The shapes of the generated score tables in Pieces.h. They mirror the lab's
// phase-scores.js one to one, so a piece reads the same in both places.
namespace score {

enum class Mode : uint8_t { Play, Rest, Build, Reduce };
enum class Voice : uint8_t { Piano, Sample, Harpsichord };

// A figure: `length` steps starting at notes[notes], MIDI numbers or -1 for a
// rest, and the order its notes are added in during a build.
struct Pattern {
  uint16_t notes, length, order, orderLength;
};

// One stage of a part, lasting `cycles` repetitions of the piece's cycle.
// `from`/`to` are an offset in steps moved between over the stage; `rate` plays
// at a multiple of the pulse. `restart` starts the figure from its first note,
// `once` plays it through once, `slips` is a chance per note of skipping or
// repeating one and staying lost.
struct Stage {
  double cycles;
  Mode mode;
  bool hasFrom;
  double from, to;
  bool hasRate;
  double rate;
  bool restart, once;
  double slips;
  uint16_t pattern;
  const char* label;
  const char* main;
  const char* word;
};

// A note of a composed piece, in beats from the start: Bach's voices are lists
// of these rather than figures on a step grid.
struct NoteEvent {
  float start, length;
  uint8_t midi;
};

// One device's part. `sample` indexes the embedded one-shots for Voice::Sample;
// `rate` plays that sample slower or faster. `main` is what the screen shows in
// place of the offset.
struct Part {
  const char* name;
  Voice voice;
  int8_t sample;
  int8_t transpose;
  double level;
  double rate;
  const char* main;
  uint16_t stage, stageCount;
  // A composed part: noteCount notes from noteEvents[note]. Zero for the
  // process pieces, which play from their stages instead.
  uint32_t note, noteCount;
};

struct Section {
  double at;
  const char* label;
};

// `cycle` is the number of pulses in one cycle of the score; `featured` is the
// part whose stage label the status line shows, or -1 for none.
struct Piece {
  const char* title;
  const char* composer;
  const char* byline;  // "after Steve Reich", or the composer and catalogue number
  uint8_t beatsPerBar; // for showing bars in composed pieces
  uint32_t periodMicros;
  uint16_t cycle;
  int8_t featured;
  uint16_t part, partCount, section, sectionCount;
};

}  // namespace score
