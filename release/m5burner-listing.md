# M5Burner listing draft

Status: **prepared locally, not submitted** (2026-10-01).

| Field | Value |
| --- | --- |
| Name | rill-chamber |
| Version | 0.1.0 |
| Device | StickS3 |
| Category | Music (if offered by the store) |
| Author | Bruce Blay |
| Source | https://github.com/bruceblay/rill-chamber |
| Browser lab | https://rillsound.com/lab |
| Cover | `release/cover.png` — 1200 × 630 |
| Additional device image | `release/device.png` — 1600 × 2000 |
| Firmware | `dist/rill-chamber-0.1.0/rill-chamber-0.1.0-factory.bin` |
| Flash address | `0x0` |
| Flash size | 8 MB |

Create a separate **rill-chamber** listing in the Rill family. Do not use the
legacy plain Rill listing or the Synth listing.

## Description

Rill Chamber turns one to four M5Stack StickS3s into a small ensemble. Play
50 pieces from the Rill Sound lab: process studies after Reich, Glass and
Rzewski, plus Bach's Inventions, Sinfonias, canons, chorales, Contrapunctus I–X
and the six-part Ricercar.

A single device plays every part. Add more devices and they share the parts,
keeping time together over ESP-NOW. No router, account or phone is needed.
Every device runs the same Chamber firmware.

The portrait display shows the selected piece, assigned parts and progress.
Piano multisamples and hand claps sit alongside synthesized harpsichord,
marimba, xylophone, glass, strings and organ.

**Controls**

- Front button: start or stop the whole Chamber ensemble.
- Side button while stopped: next piece; hold for the next collection.
- Side button while playing: cycle this device's volume.

Switch on all players, allow a couple of seconds for discovery, then press
Play. Playback starts after a 2.5-second lead-in. Devices joining during a
performance take parts at the next stop/start. At the end, press the front
button to stop, then press again to replay.

For M5Stack StickS3 only. Chamber uses its own ensemble protocol; it does not
share playback controls or timing with Voice, Synth, Mallet, World or Drums.
Volume is reduced automatically when the battery is low. Piece selection and
volume reset on reboot.

Source, build instructions, score and sample credits:
https://github.com/bruceblay/rill-chamber

## Version notes

Initial release: 50 pieces, seven collections, solo or ensemble playback on
up to four StickS3s, shared controls and local volume.

## Before submission

See [release readiness](READINESS.md). The factory image replaces the device's
firmware and starts with empty settings. Upload the factory image at `0x0`,
not the app-only image. Use the browser store publisher; the desktop
M5Burner app is not needed for preparation.
