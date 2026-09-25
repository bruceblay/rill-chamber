// Copyright (c) 2026 Bruce Blay
// SPDX-License-Identifier: GPL-3.0-or-later
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "Score.h"
#include "score_vectors.h"

// Every note of every part must land where the lab's JavaScript engine puts
// it: same count, same pitch, same time to a millionth of a pulse.

static int failures = 0;
static void check(bool ok, const char* what, unsigned partIndex, unsigned note) {
  if (ok) return;
  if (++failures <= 10) std::printf("FAIL %s: part %u note %u\n", what, partIndex, note);
}

int main() {
  unsigned partIndex = 0;
  for (unsigned p = 0; p < score::pieceCount; ++p) {
    const score::Piece& piece = score::piece(p);
    for (unsigned i = 0; i < piece.partCount; ++i, ++partIndex) {
      struct Out { double time; int note; };
      std::vector<Out> out;
      score::PartPlayer player;
      player.reset(1);
      for (int64_t local = 0; player.pulse(piece, score::part(piece, i), local, [&](double time, int note) { out.push_back({time, note}); }); ++local) {}
      const Span& span = spans[partIndex];
      check(out.size() == span.count, "note count", partIndex, unsigned(out.size()));
      for (unsigned n = 0; n < std::min<size_t>(out.size(), span.count); ++n) {
        const Hit& hit = hits[span.start + n];
        check(std::fabs(out[n].time - hit.time) < 1e-5, "time", partIndex, n);
        check(out[n].note == hit.note, "pitch", partIndex, n);
      }
      std::printf("%-15s %-8s %5zu notes\n", piece.title, score::part(piece, i).name, out.size());
    }
  }
  // Dealing parts: two devices split a duo, one device plays everything.
  check(score::plays(0, 0, 2) && score::plays(1, 1, 2) && !score::plays(1, 0, 2), "duo deal", 0, 0);
  check(score::plays(0, 0, 1) && score::plays(1, 0, 1), "solo deal", 0, 0);
  check(score::plays(3, 1, 2) && score::plays(2, 0, 2), "four parts on two", 0, 0);
  if (failures) { std::printf("%d failures\n", failures); return 1; }
  std::printf("score: all parts match the lab\n");
  return 0;
}
