# Building and packaging

## Firmware

Use Python with a virtual environment and the pinned PlatformIO version:

```sh
python3 -m venv .venv
.venv/bin/python -m pip install -r requirements-dev.txt
.venv/bin/python -m platformio run
```

PlatformIO installs Espressif32 6.12.0 (Arduino ESP32 2.0.17), M5Unified
0.2.21 and M5GFX 0.2.28. The build targets the StickS3's ESP32-S3, 8 MB
flash and OPI PSRAM. `VERSION` is embedded in the serial startup log.

To install on a device you explicitly select:

```sh
.venv/bin/python tools/flash.py --port /dev/cu.usbmodemYOUR_DEVICE
```

The serial monitor runs at 115200 baud. The firmware starts stopped and
prints `rill-chamber 0.1.0`, the device ID, radio status and piece count.

## Release package

Commit the intended source, then run:

```sh
.venv/bin/python tools/package_release.py
```

The packager refuses a dirty source tree or an existing output directory,
rebuilds the firmware, and creates `dist/rill-chamber-VERSION/` with:

- An 8 MB factory image for M5Burner at address `0x0`.
- The application image for developers (address `0x10000`; needs matching bootloader/partitions).
- The exact Git source archive, including generated scores and samples.
- Cover, listing, README, changelog and attribution documents.
- A manifest recording commit, tool versions, flash segments and SHA-256 hashes.
- `SHA256SUMS` for all packaged files.

The image is built from files, never from a device flash dump. NVS and unused
flash are erased bytes. The packager checks segment bounds, application size,
blank NVS and the merged application bytes. It does not flash a device or
publish to a store.

## Host tools

The existing suite can be run explicitly when checking engine changes:

```sh
python3 tools/test.py --sanitize
```

It compares score timing with exported browser vectors, renders representative
pieces, and exercises roster and shared-clock behavior. Host checks do not
establish speaker quality or radio timing on physical devices.

To export scores again, place `rill-sound` beside this checkout and run
`node tools/export_scores.mjs`. This rewrites `src/Pieces.h` and the score
vectors. `tools/embed_samples.py` rebuilds the piano and clap data using the
sibling repositories and FFmpeg. Neither operation is needed for a normal
build: the generated headers are committed here.
