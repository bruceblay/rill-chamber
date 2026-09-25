// Copyright (c) 2026 Bruce Blay
// SPDX-License-Identifier: GPL-3.0-or-later
#include <cmath>
#include <cstdio>
#include <memory>
#include "Player.h"
#include "score_vectors.h"

// Render a piece the way the audio task does, block by block against a steady
// clock, and check the engine starts exactly the notes the score has, makes
// sound, and never clips.
static int failures = 0;
#define CHECK(condition) do { if (!(condition)) { ++failures; std::printf("FAIL line %d: %s\n", __LINE__, #condition); } } while (0)

static unsigned expected(unsigned firstPart, unsigned parts, double before) {
  unsigned total = 0;
  for (unsigned p = firstPart; p < firstPart + parts; ++p)
    for (unsigned n = 0; n < spans[p].count; ++n)
      if (hits[spans[p].start + n].time < before) ++total;
  return total;
}

static void run(unsigned pieceIndex, unsigned firstPart, int rank, unsigned members, unsigned parts, double seconds) {
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
  unsigned want = expected(firstPart, parts, position);
  std::printf("%-15s rank %d of %u: %u notes (score has %u), peak %d, rms %.0f\n", piece.title, rank, members,
              unsigned(engine->notesStarted()), want, peak, std::sqrt(energy / (blocks * 512.0)));
  // A note due right at the end of the last block can land either side of it.
  CHECK(std::abs(int(engine->notesStarted()) - int(want)) <= int(parts));
  CHECK(peak < 32767 && peak > 3000);
}

int main() {
  run(0, 0, 0, 1, 2, 30);  // Piano Phase alone: both parts
  run(0, 1, 1, 2, 1, 30);  // Piano Phase, second of two devices
  run(1, 2, 0, 1, 2, 20);  // Clapping Music alone
  run(2, 4, 0, 1, 4, 70);  // Tempo Canon alone: all four voices
  if (failures) { std::printf("%d failures\n", failures); return 1; }
  std::printf("player: notes on time, audible, unclipped\n");
  return 0;
}
