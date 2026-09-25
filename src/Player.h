// Copyright (c) 2026 Bruce Blay
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <mutex>
#include "Samples.h"
#include "Score.h"

// Turns this device's parts into sound. The audio task hands render() the
// score position the shared clock says the block should start at; the engine
// keeps its own position and steers it there gently, so a correction is never
// heard as a stumble. Notes come from the score engine a pulse ahead and are
// started at their exact sample within the block.
namespace player {

constexpr uint32_t rate = 32000;
constexpr unsigned maxParts = 4;

// What the display needs about one part, copied out of the audio task.
struct View {
  bool active = false, done = false;
  unsigned part = 0, stage = 0;
  double phase = 0, progress = 0;
  int64_t step = 0, position = 0;
  int slip = 0;
  float flash = 0;
};

class Engine {
 public:
  // Which piece, and which of its parts this device plays.
  void configure(unsigned pieceIndex, int rank, unsigned members, uint32_t seed) {
    piece_ = &score::piece(pieceIndex);
    partCount_ = 0;
    if (rank >= 0)
      for (unsigned i = 0; i < piece_->partCount && partCount_ < maxParts; ++i)
        if (score::plays(i, unsigned(rank), members)) parts_[partCount_++] = i;
    for (unsigned i = 0; i < partCount_; ++i) players_[i].reset(seed * 2654435761u + i + 1);
    periodSamples_ = double(piece_->periodMicros) * rate / 1e6;
    pending_ = 0;
    scheduled_ = 0;
    tracking_ = false;
    finished_ = false;
    flash_.fill(0);
  }
  // Stop starting notes; whatever is ringing fades out.
  void silence() {
    piece_ = nullptr;
    for (auto& v : voices_) if (v.on && v.releaseLeft == 0) v.releaseLeft = releaseSamples;
  }

  unsigned partCount() const { return partCount_; }
  bool finished() const { return finished_; }
  uint32_t notesStarted() const { return started_; }
  // Diagnostics for the serial log: voices cut off to make room, jumps taken
  // outright instead of steered, notes dropped for arriving too late, and the
  // most voices ever sounding at once.
  uint32_t steals() const { return steals_; }
  uint32_t snaps() const { return snapsAhead_ + snapsBack_; }
  uint32_t snapsAhead() const { return snapsAhead_; }
  uint32_t snapsBack() const { return snapsBack_; }
  double worstError() const { return worstError_; }
  // Output level over the last blocks, 0 to 1, for comparing devices.
  float level() const { return level_; }
  uint32_t lateNotes() const { return late_; }
  unsigned peakVoices() const { return peakVoices_; }

  // `target` is the score position, in pulses, at this block's first sample.
  void render(int16_t* out, unsigned count, double target) {
    // The mix lives in the engine, not on the audio task's small stack.
    std::array<float, 1024>& mix = mix_;
    count = std::min<unsigned>(count, mix.size());
    std::fill(mix.begin(), mix.begin() + count, 0.0f);
    if (piece_) advance(count, target);
    unsigned sounding = 0;
    for (auto& v : voices_) if (v.on) { play(v, mix.data(), count); ++sounding; }
    peakVoices_ = std::max(peakVoices_, sounding);
    double energy = 0;
    for (unsigned i = 0; i < count; ++i) { out[i] = finish(mix[i]); energy += double(out[i]) * out[i]; }
    level_ += (float(std::sqrt(energy / count) / 32768.0) - level_) * 0.01f;
    for (auto& f : flash_) f *= 0.8f;
    snapshot();
  }

  void setMaster(float master) { master_ = master; }

  // The output stage, made to be as loud as the speaker allows. A high-pass
  // below about 200 Hz, since the speaker cannot move air down there and bass
  // only uses up headroom; then plenty of gain into a limiter that catches
  // peaks instantly and lets go over 80 ms; then a soft clip for anything the
  // limiter's first sample lets through.
  int16_t finish(float input) {
    float x = input * master_;
    highpass_ = 0.9622f * (highpass_ + x - lastInput_);
    lastInput_ = x;
    x = highpass_;
    envelope_ = std::max(std::fabs(x), envelope_ * 0.99961f);
    if (envelope_ > 0.92f) x *= 0.92f / envelope_;
    x = std::clamp(x, -1.5f, 1.5f);
    return int16_t((x - x * x * x * (4.0f / 27.0f)) * 32767.0f);
  }

  // The display's copy. Never blocks the audio task: if the display is reading,
  // this block simply skips updating it.
  unsigned views(View* out) {
    std::lock_guard<std::mutex> hold(viewLock_);
    std::copy(views_.begin(), views_.begin() + viewCount_, out);
    return viewCount_;
  }

 private:
  static constexpr unsigned releaseSamples = rate / 7;
  static constexpr unsigned pianoHold = rate * 9 / 10;
  // The lab's harpsichord: partials at these multiples of the note, each with
  // its level and the seconds it takes to die away. Strong upper partials
  // that fade quickly are what make it read as plucked.
  static constexpr unsigned partialCount = 7;
  static constexpr float partials[partialCount][3] = {
    {1, 1, 1.1f}, {2, 0.75f, 0.7f}, {3, 0.5f, 0.45f}, {4, 0.35f, 0.3f},
    {5, 0.22f, 0.2f}, {6, 0.14f, 0.14f}, {8, 0.08f, 0.08f}};
  static constexpr unsigned sineSize = 1024;
  struct Voice {
    const int16_t* data = nullptr;
    // Synthesized voices sum decaying partials instead of reading a sample.
    bool synth = false;
    unsigned partialsUsed = 0;
    std::array<float, partialCount> amplitude{}, fade{}, phase{}, increment{};
    uint32_t length = 0, delay = 0, hold = 0, releaseLeft = 0;
    double position = 0, step = 1;
    float gain = 0;
    bool on = false;
  };
  struct Note { double time; int note; uint8_t part; double length; };

  void advance(unsigned count, double target) {
    // A big difference is a start or a jump: take it outright. A small one is
    // clock drift: pay a fifth of it off per block.
    const double error = target - position_;
    if (tracking_) worstError_ = std::max(worstError_, std::fabs(error));
    if (!tracking_) {
      position_ = target;
      scheduled_ = std::max<int64_t>(0, int64_t(std::floor(target)));
      tracking_ = true;
    } else if (std::fabs(error) > 1.0) {
      // A jump of more than a pulse: take the new position, but never re-run
      // pulses already scheduled. Rewinding would play their notes a second
      // time, which is heard as the part lurching.
      ++(error > 0 ? snapsAhead_ : snapsBack_);
      position_ = target;
      scheduled_ = std::max(scheduled_, std::max<int64_t>(0, int64_t(std::floor(target))));
    } else {
      position_ += error * 0.2;
    }
    const double span = count / periodSamples_;
    const double end = position_ + span;
    bool anyPlaying = false;
    while (double(scheduled_) <= std::floor(end)) {
      anyPlaying = false;
      for (unsigned i = 0; i < partCount_; ++i) {
        const score::Part& part = score::part(*piece_, parts_[i]);
        anyPlaying |= players_[i].pulse(*piece_, part, scheduled_, [&](double time, int note, double length) {
          if (pending_ < pendingNotes_.size()) pendingNotes_[pending_++] = {time, note, uint8_t(i), length};
        });
      }
      if (!anyPlaying && scheduled_ >= 0 && partCount_) finished_ = true;
      ++scheduled_;
    }
    unsigned kept = 0;
    for (unsigned n = 0; n < pending_; ++n) {
      const Note& note = pendingNotes_[n];
      if (note.time >= end) { pendingNotes_[kept++] = note; continue; }
      if (note.time < position_ - 0.5) { ++late_; continue; }  // too late to be in time
      uint32_t offset = uint32_t(std::clamp((note.time - position_) * periodSamples_, 0.0, double(count - 1)));
      start(note, offset);
    }
    pending_ = kept;
    position_ = end;
  }

  void start(const Note& note, uint32_t offset) {
    const score::Part& part = score::part(*piece_, parts_[note.part]);
    Voice* v = nullptr;
    for (auto& candidate : voices_) if (!candidate.on) { v = &candidate; break; }
    if (!v) {
      ++steals_;
      // All busy: take the one furthest through its sound.
      v = &*std::max_element(voices_.begin(), voices_.end(), [](const Voice& a, const Voice& b) {
        return a.position / a.length < b.position / b.length;
      });
    }
    *v = Voice{};
    v->on = true;
    v->delay = offset;
    const double spread = double(rng() >> 8) / 16777216.0;
    const float share = float(part.level / std::sqrt(double(piece_->partCount)));
    // A composed note stops at its written length, as a finger lifts from the
    // key; the process pieces' notes ring out.
    const uint32_t written = note.length > 0 ? uint32_t(std::max(0.08 * rate, note.length * periodSamples_)) : 0;
    if (part.voice == score::Voice::Harpsichord) {
      const double hz = 440.0 * std::pow(2.0, (note.note + part.transpose - 69) / 12.0);
      v->synth = true;
      for (unsigned p = 0; p < partialCount; ++p) {
        const double f = hz * partials[p][0];
        if (f > rate * 0.4) break;
        v->amplitude[p] = partials[p][1];
        // Decays to 5% over the partial's time, as setTargetAtTime does.
        v->fade[p] = float(std::exp(-3.0 / (partials[p][2] * rate)));
        v->increment[p] = float(f * sineSize / rate);
        v->phase[p] = 0;
        v->partialsUsed = p + 1;
      }
      v->length = rate * 3;
      v->hold = written ? written : rate * 2;
      v->gain = share * float(0.6 + 0.08 * spread) * 0.22f;
    } else if (part.voice == score::Voice::Sample) {
      const samples::Clip& clip = samples::oneShots[part.sample];
      v->data = clip.data;
      v->length = clip.length;
      v->step = part.rate;
      v->hold = clip.length;
      v->gain = share * float(0.75 + 0.25 * spread) * 0.7f;
    } else {
      // Nearest zone, then resample from its root, as Rill Mallet does.
      const int midi = note.note + part.transpose;
      const samples::Zone* zone = &samples::piano[0];
      for (const auto& z : samples::piano)
        if (std::abs(int(z.root) - midi) < std::abs(int(zone->root) - midi)) zone = &z;
      v->data = zone->data;
      v->length = zone->length;
      v->step = std::pow(2.0, double(midi - int(zone->root)) / 12.0);
      v->hold = written ? std::min(written, pianoHold) : pianoHold;
      v->gain = share * float(0.55 + 0.1 * spread) * 0.8f;
    }
    flash_[note.part] = 1;
    ++started_;
  }

  float synthesize(Voice& v) {
    float sum = 0;
    for (unsigned p = 0; p < v.partialsUsed; ++p) {
      unsigned index = unsigned(v.phase[p]);
      float frac = v.phase[p] - float(index);
      sum += v.amplitude[p] * (sine_[index] + (sine_[index + 1] - sine_[index]) * frac);
      v.amplitude[p] *= v.fade[p];
      v.phase[p] += v.increment[p];
      if (v.phase[p] >= float(sineSize)) v.phase[p] -= float(sineSize);
    }
    return sum;
  }

  void play(Voice& v, float* mix, unsigned count) {
    for (unsigned i = 0; i < count; ++i) {
      if (v.delay) { --v.delay; continue; }
      float sample;
      if (v.synth) {
        if (v.position >= v.length) { v.on = false; return; }
        sample = synthesize(v);
        v.position += 1;
      } else {
        uint32_t index = uint32_t(v.position);
        if (index + 1 >= v.length) { v.on = false; return; }
        float frac = float(v.position - index);
        sample = (v.data[index] + (v.data[index + 1] - v.data[index]) * frac) / 32768.0f;
        v.position += v.step;
      }
      float envelope = 1;
      if (v.hold) --v.hold;
      else if (!v.releaseLeft) v.releaseLeft = releaseSamples;
      if (v.releaseLeft) {
        envelope = float(v.releaseLeft) / releaseSamples;
        if (--v.releaseLeft == 0) { v.on = false; return; }
      }
      mix[i] += sample * v.gain * envelope;
    }
  }

  void snapshot() {
    std::unique_lock<std::mutex> hold(viewLock_, std::try_to_lock);
    if (!hold.owns_lock()) return;
    viewCount_ = partCount_;
    for (unsigned i = 0; i < partCount_; ++i) {
      const score::Where& where = players_[i].where();
      View& view = views_[i];
      view.active = where.stage != nullptr;
      view.done = where.done;
      view.part = parts_[i];
      view.stage = where.index;
      view.phase = where.phase;
      view.progress = where.progress;
      view.step = players_[i].step();
      view.position = scheduled_;
      view.slip = players_[i].slip();
      view.flash = flash_[i];
    }
  }

  uint32_t rng() { rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5; return rng_; }

  const score::Piece* piece_ = nullptr;
  std::array<unsigned, maxParts> parts_{};
  std::array<score::PartPlayer, maxParts> players_{};
  unsigned partCount_ = 0;
  double periodSamples_ = 3200, position_ = 0;
  int64_t scheduled_ = 0;
  bool tracking_ = false, finished_ = false;
  std::array<Note, 64> pendingNotes_{};
  unsigned pending_ = 0;
  std::array<Voice, 40> voices_{};
  std::array<float, 1024> mix_{};
  // One cycle of a sine, with a guard sample for interpolation.
  std::array<float, sineSize + 1> sine_ = [] {
    std::array<float, sineSize + 1> table{};
    for (unsigned i = 0; i <= sineSize; ++i) table[i] = float(std::sin(2 * 3.14159265358979 * i / sineSize));
    return table;
  }();
  std::array<float, maxParts> flash_{};
  float master_ = 3.0f, highpass_ = 0, lastInput_ = 0, envelope_ = 0;
  uint32_t rng_ = 0x9e3779b9, started_ = 0, steals_ = 0, snapsAhead_ = 0, snapsBack_ = 0, late_ = 0;
  double worstError_ = 0;
  float level_ = 0;
  unsigned peakVoices_ = 0;
  std::mutex viewLock_;
  std::array<View, maxParts> views_{};
  unsigned viewCount_ = 0;
};

}  // namespace player
