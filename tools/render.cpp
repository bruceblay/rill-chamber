// Copyright (c) 2026 Bruce Blay
// SPDX-License-Identifier: GPL-3.0-or-later
// Renders a piece through the device's own audio engine to a WAV file, so the
// firmware's sound can be heard without a device:
//   c++ -std=c++17 -O2 -I src tools/render.cpp -o render && ./render 0 60 piece.wav
// Arguments: piece index, seconds, output file, and optionally the device's
// rank and member count (default: one device playing every part).
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>
#include "Player.h"

int main(int argc, char** argv) {
  if (argc < 4) { std::fprintf(stderr, "usage: render piece seconds out.wav [rank members]\n"); return 2; }
  unsigned pieceIndex = unsigned(std::atoi(argv[1]));
  double seconds = std::atof(argv[2]);
  int rank = argc > 4 ? std::atoi(argv[4]) : 0;
  unsigned members = argc > 5 ? unsigned(std::atoi(argv[5])) : 1;
  auto engine = std::make_unique<player::Engine>();
  engine->configure(pieceIndex, rank, members, 1);
  const score::Piece& piece = score::piece(pieceIndex);
  const double periodSamples = double(piece.periodMicros) * player::rate / 1e6;
  std::vector<int16_t> out;
  double position = -4;
  int16_t block[512];
  while (out.size() < seconds * player::rate) {
    engine->render(block, 512, position);
    position += 512 / periodSamples;
    out.insert(out.end(), block, block + 512);
  }
  FILE* file = std::fopen(argv[3], "wb");
  if (!file) return 1;
  auto u32 = [&](uint32_t v) { std::fwrite(&v, 4, 1, file); };
  auto u16 = [&](uint16_t v) { std::fwrite(&v, 2, 1, file); };
  std::fwrite("RIFF", 1, 4, file); u32(36 + out.size() * 2); std::fwrite("WAVEfmt ", 1, 8, file);
  u32(16); u16(1); u16(1); u32(player::rate); u32(player::rate * 2); u16(2); u16(16);
  std::fwrite("data", 1, 4, file); u32(out.size() * 2);
  std::fwrite(out.data(), 2, out.size(), file);
  std::fclose(file);
  std::printf("%s: %.0f s of %s\n", argv[3], seconds, piece.title);
}
