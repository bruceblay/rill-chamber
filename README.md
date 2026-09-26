# Rill Chamber

**rill** /rɪl/ *noun*: a small stream or a tiny, shallow channel cut into soil by running water.

Process music for several StickS3s, one part each: the pieces from the
[Rill Sound lab](https://rillsound.com/lab) for up to four devices, after
Reich, Glass and Rzewski and by Johann Sebastian Bach, on real devices keeping
time together over ESP-NOW.

Every device runs this same firmware. Switch on two or more near each other and
they find each other within a second: the lowest id keeps the clock, and every
device knows who else is present. A press on any device starts or stops the
piece for all of them.

Parts are dealt out by rank, so any piece plays on one to four devices. One
device alone plays every part, which is handy for trying a piece; with four,
each device takes one part of a four-part piece. With fewer devices than
parts, each takes several.

## Pieces

Browsed a collection at a time.

| Collection | Pieces | Voices |
| --- | --- | --- |
| Process | Piano Phase, Clapping Music, Violin Phase (after Reich); Music in Fifths (after Glass); Les Moutons de Panurge (after Rzewski) | 2–4 |
| Inventions | Bach's fifteen Two-Part Inventions, BWV 772–786 | 2 |
| Sinfonias | Bach's fifteen Three-Part Sinfonias, BWV 787–801 | 3 |
| The Art of Fugue | Contrapunctus I–X, BWV 1080 | 3–4 |
| Canons | Goldberg Variation 3; the canon by augmentation, BWV 1079 | 3 |
| The Musical Offering | Ricercar a 6, BWV 1079 | 6 |
| Chorales | BWV 269 and 347 | 4 |

For the process pieces the figures are original to the lab and the processes
are the composers'. The Bach pieces are Bach's own notes, from the public-domain and
CC BY-SA editions listed in `rill-sound/reference/bach/SOURCES.md`, played
on a harpsichord synthesized as the lab's is, each note held for its written
length. The scores are not written here: `tools/export_scores.mjs` reads
them from the lab (`rill-sound/src/lab/phase-scores.js` and `public/bach`)
into `src/Pieces.h`, so the browser and the device play the same pieces.

## Controls

- Front button: play or stop, for every device.
- Side button: next piece in the collection while stopped; hold it for the
  next collection. Volume while playing.

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

Piano: Rill Mallet's multisamples (VCSL, CC0). Harpsichord, marimba,
xylophone and glass are synthesized from decaying partials, and strings and
organ from filtered harmonics, all as the lab's `voices.js` makes them. Claps: two single hand claps
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

## Credits and license

Created by Bruce Blay.

GPL-3.0-or-later, as the rest of the Rill family.
