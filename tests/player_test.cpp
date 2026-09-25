// Copyright (c) 2026 Bruce Blay
// SPDX-License-Identifier: GPL-3.0-or-later
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include "Player.h"
#include "score_vectors.h"

// Render a piece the way the audio task does, block by block against a steady
// clock, and check the engine starts exactly the notes the score has, makes
// sound, and never clips.
static int failures = 0;
#define CHECK(condition) do { if (!(condition)) { ++failures; std::printf("FAIL line %d: %s\n", __LINE__, #condition); } } while (0)

// The score's notes before `before` for the parts this device is dealt, or -1
// if one of them slips at random and has no fixed count.
static int expected(const score::Piece& piece, int rank, unsigned members, double before) {
  int total = 0;
  for (unsigned i = 0; i < piece.partCount; ++i) {
    if (!score::plays(i, unsigned(rank), members)) continue;
    const Span& span = spans[piece.part + i];
    if (span.count == RANDOM) return -1;
    for (unsigned n = 0; n < span.count; ++n)
      if (hits[span.start + n].time < before) ++total;
  }
  return total;
}

static unsigned find(const char* title) {
  for (unsigned i = 0; i < score::pieceCount; ++i)
    if (!std::strcmp(score::piece(i).title, title)) return i;
  std::printf("no piece %s\n", title);
  std::exit(1);
}

static void run(const char* title, int rank, unsigned members, double seconds) {
  const unsigned pieceIndex = find(title);
  auto engine = std::make_unique<player::Engine>();
  engine->configure(pieceIndex, rank, members, 7);
  const score::Piece& piece = score::piece(pieceIndex);
  const double periodSamples = double(piece.periodMicros) * player::rate / 1e6;
  int16_t block[512];
  double position = -2;  // a lead-in, as on the device
  int peak = 0;
  double energy = 0;
  const unsigned blocks = unsigned(seconds * player::rate / 512);
  for (unsigned b = 0; b < blocks; ++b) {
    engine->render(block, 512, position);
    position += 512 / periodSamples;
    for (int16_t sample : block) { peak = std::max(peak, std::abs(int(sample))); energy += double(sample) * sample; }
  }
  const int want = expected(piece, rank, members, position);
  std::printf("%-22s rank %d of %u: %u notes (score has %d), peak %d, rms %.0f\n", piece.title, rank, members,
              unsigned(engine->notesStarted()), want, peak, std::sqrt(energy / (blocks * 512.0)));
  // A note due right at the end of the last block can land either side of it.
  if (want >= 0) CHECK(std::abs(int(engine->notesStarted()) - want) <= int(piece.partCount));
  else CHECK(engine->notesStarted() > 0);
  CHECK(peak < 32767 && peak > 3000);
}

int main() {
  run("Piano Phase", 0, 1, 30);           // alone: both parts
  run("Piano Phase", 1, 2, 30);           // second of two devices
  run("Clapping Music", 0, 1, 20);
  run("Violin Phase", 0, 4, 30);          // one violin each on four devices
  run("Violin Phase", 0, 1, 30);          // all four on one device
  run("Music in Fifths", 1, 3, 30);       // one organ of three
  run("Music in Fifths", 0, 1, 30);
  run("Les Moutons de Panurge", 3, 4, 30);  // the glass, on the fourth device
  run("Les Moutons de Panurge", 0, 1, 30);
  run("Invention No. 1", 0, 2, 40);       // upper voice on the first of two
  run("Invention No. 1", 0, 1, 40);       // both voices on one device
  run("Sinfonia No. 9", 1, 2, 40);        // a three-voice piece on two devices
  run("Goldberg Variation 3", 0, 1, 40);
  run("Canon by Augmentation", 0, 1, 40); // ornaments and all
  run("Contrapunctus I", 0, 1, 40);       // all four voices on one device
  run("Ricercar a 6", 0, 1, 40);          // all six voices on one device
  if (failures) { std::printf("%d failures\n", failures); return 1; }
  std::printf("player: notes on time, audible, unclipped\n");
  return 0;
}
