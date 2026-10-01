# Rill Chamber

**rill** /rɪl/ *noun*: a small stream or a tiny, shallow channel cut into soil by running water.

**Version 0.1.0 — prepared for M5Burner; not yet submitted.**

Process studies after Reich, Glass and Rzewski, and music by Johann Sebastian
Bach, from the [Rill Sound lab](https://rillsound.com/lab). One to four M5Stack
StickS3s share the parts and keep time together over ESP-NOW.

Every device runs this same firmware. Switch on two or more near each other and
allow a couple of seconds for discovery: the lowest id keeps the clock, and every
device knows who else is present. A press on any device starts or stops the
piece for all of them.

Parts are dealt out by rank, so any piece plays on one to four devices. One
device alone plays every part, which is handy for trying a piece; with four,
each device takes one part of a four-part piece. With fewer devices than
parts, each takes several.

## Pieces

50 pieces, browsed a collection at a time.

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
CC BY-SA editions listed in [score and sample credits](docs/SOURCES.md), played
on a harpsichord synthesized as the lab's is, each note held for its written
length. The scores are not written here: `tools/export_scores.mjs` reads
them from the lab (`rill-sound/src/lab/phase-scores.js` and `public/bach`)
into `src/Pieces.h`, so the browser and the device play the same pieces.

## Controls

| Button | While stopped | While playing |
| --- | --- | --- |
| Front | Start the whole ensemble, after a 2.5-second lead-in | Stop the whole ensemble |
| Side, tap | Next piece in this collection, for everyone | Local volume: 100% → 67% → 43% → 100% |
| Side, hold (0.6 seconds) | Next collection, for everyone | No action |

The device starts stopped, on Piano Phase. Piece and volume are not saved
across reboots. A low battery automatically caps volume; the screen indicates
the cap. There are no shake or tilt controls.

At the end of a piece the screen shows DONE. Press Front to stop; press it
again to replay. Stop before browsing to another piece.

## Playing together

Install the same Chamber version on each device. Switch on all players near
each other, wait a couple of seconds, choose a piece and press Front on any
player. Parts are assigned when playback starts, by device ID rather than
physical position. More players than parts means some devices listen only.
The six-part Ricercar shares its six parts among at most four devices.

Membership stays fixed during a performance. To include a device that joined
late, stop and start again. If a player leaves, its parts remain silent until
the next start redistributes them.

Chamber communicates directly over ESP-NOW on Wi-Fi channel 1. No router or
internet is required. It uses a separate protocol from the five Rill Sound
instruments, so those instruments do not join a Chamber performance.

## How it keeps time

The clock comes from Rill Sync (`src/Ensemble.h`), with its own packet magic so a chamber ensemble
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
[credits](docs/SOURCES.md)). `tools/embed_samples.py` writes
`src/Samples.h` from both.

## Build and install

The M5Burner listing is being prepared under **rill-chamber**. It has not
been submitted in this release preparation. Use [the source build](docs/BUILD.md)
or the prepared factory image until it is available.

```sh
python3 -m venv .venv
.venv/bin/python -m pip install -r requirements-dev.txt
.venv/bin/python -m platformio run
.venv/bin/python tools/flash.py --port /dev/cu.usbmodemYOUR_DEVICE
```

See [build and packaging instructions](docs/BUILD.md), the
[store listing draft](release/m5burner-listing.md), and
[release readiness](release/READINESS.md).

## Credits and license

Created by Bruce Blay.

Firmware code: [GPL-3.0-or-later](LICENSE). Score editions and recordings
retain their own licenses; see [sources and attribution](docs/SOURCES.md).
