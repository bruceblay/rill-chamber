# Rill Chamber

Process music for several StickS3s, one part each: the pieces from the
[Rill Sound lab](https://rillsound.com/lab), after Steve Reich and Conlon
Nancarrow, on real devices keeping time together over ESP-NOW.

Every device runs this same firmware. Switch on two or more near each other and
they find each other within a second: the lowest id keeps the clock, and every
device knows who else is present. A press on any device starts or stops the
piece for all of them.

Parts are dealt out by rank, so any piece plays on any number of devices. One
device alone plays every part, which is handy for trying a piece; two devices
share a duo; two devices playing a four-part piece take two parts each.

## Pieces

| Piece | Parts | After |
| --- | --- | --- |
| Piano Phase | 2 | Steve Reich, *Piano Phase* (1967) |
| Clapping Music | 2 | Steve Reich, *Clapping Music* (1972) |
| Tempo Canon | 4 | Conlon Nancarrow, *Study for Player Piano No. 37* |

The figures and melodies are original to the lab; the processes are the
composers'. The scores are not written here: `tools/export_scores.mjs` reads
them from the lab (`rill-sound/src/lab/phase-scores.js`) into `src/Pieces.h`,
so the browser and the device play the same pieces.

## Controls

- Front button: play or stop, for every device.
- Side button: next piece while stopped; volume while playing.

## How it keeps time

The clock is Rill Sync's (`src/Ensemble.h`), measured on two devices at about
a third of a millisecond apart, with its own packet magic so a chamber ensemble
never mixes with a Mallet or Drums one. It gives every device the same timeline.
A start is a proposal to begin pulse 0 of the score two and a half seconds out
on that timeline, and every device carries the whole score, so each works out
its own notes from the time alone.

A part that phases plays deliberately off the pulse, moving its offset at a
steady rate and relocking a note ahead, and the engine solves for each note's
exact time rather than stepping it.

## Sounds

Piano: Rill Mallet's multisamples (VCSL, CC0). Claps: two single hand claps
cut from a CC0 flamenco palmas recording (see
`rill-sound/public/licenses/samples.txt`). `tools/embed_samples.py` writes
`src/Samples.h` from both.

## Build and install

```sh
python tools/test.py                           # host tests
python -m platformio run                        # build
python tools/flash.py --port /dev/cu.usbmodem…  # install
```

`python tools/test.py` checks the C++ engine against every note time the lab's
JavaScript engine produces (`tests/score_vectors.h`, written by the exporter),
so the two cannot drift apart unnoticed. `tools/render.cpp` renders a piece
through the device's own audio engine to a WAV file.

## Licence

GPL-3.0-or-later, as the rest of the Rill family.
