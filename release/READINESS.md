# Chamber 0.1.0 release readiness

Prepared on 2026-10-01. **Local release candidate; not submitted to M5Burner.**

## Completed in this preparation

- Reviewed firmware controls, ensemble state, audio-task transitions and display handoff.
- Corrected a stop transition that could leave the score engine active.
- Reject foreign protocol packets and malformed shared control states before use.
- Associate display snapshots with their piece; copy audio diagnostics under a lock.
- Built for StickS3 with the pinned PlatformIO and library versions.
- Added a version, changelog, self-contained build instructions and asset attribution.
- Prepared the store description and 1200 × 630 cover.
- Packaged the 8 MB factory image from committed source `3187fe0`; blank NVS,
  segment bounds, merged bytes and unused flash checks passed.
- Recorded artifact hashes in [the manifest](0.1.0-manifest.json). The build used
  2,466,705 bytes of the 7 MiB application partition and 73,968 bytes of static RAM.

The cover uses the Rill Sound social cards' palette and circular composition,
with note data from Bach's Contrapunctus I behind the shared 3D StickS3 model.
The device display is rendered from Chamber's playing-screen code. Chamber does not yet have a dedicated web-app meta image; this
cover is a local candidate for that shared asset.

## Not performed in this preparation

The host test suite was not run. No device was flashed, and audio quality,
radio timing, battery behavior and multi-device controls were not exercised
on hardware. No M5Burner listing was created or changed.

## Hardware acceptance before public submission

1. Flash the factory image to a StickS3. Confirm version 0.1.0 in the serial
   log, successful radio startup, and the stopped Piano Phase menu.
2. Start, stop during sounding notes, restart, and change pieces/collections
   repeatedly. Confirm stopping releases the audio and the display remains valid.
3. Play all five process studies and representative Bach works, including
   the six-part Ricercar on one device. Listen for breakup and missed notes.
4. With two to four devices, start and stop from each device. Confirm parts
   are divided once, with shared starts and stable timing during a long piece.
5. Join late, restart the conductor, remove one player, then stop/start to
   redistribute parts. Confirm the documented membership behavior.
6. Check all three local volume steps, battery operation, low-voltage volume
   cap and reboot behavior. Confirm DONE and replay controls at the end.

Record the device count, firmware hash, duration and observations here before
marking hardware acceptance complete. Use the browser publisher for submission.
