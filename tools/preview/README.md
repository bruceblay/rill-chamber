# Chamber device image

`screen.cpp` calls the firmware's `screen::playing` for Contrapunctus I,
soprano, beat 68, with volume 67%. The host adapter renders the playing
screen using the M5GFX bitmap fonts and RGB565 colors. It does not emulate
hardware, radio or audio. The menu's Font2 is not implemented in the adapter.

After a PlatformIO build has installed M5GFX:

```sh
c++ -std=c++17 -O2 -I tools/preview -I src \
  -I .pio/libdeps/sticks3/M5GFX@0.2.28/src/lgfx/Fonts \
  tools/preview/screen.cpp -o build/preview-screen
build/preview-screen build/chamber-screen.ppm
```

Convert the PPM to PNG without resizing. Save it as
`release/chamber-screen.png` and copy it to the sibling Rill Sound checkout
at `scripts/social-art/chamber.png`.

Run Rill Sound's Vite dev server and open
`/scripts/chamber-device-preview.html`. It imports the same `createStickS3`
CAD model and studio lighting used by the other social covers. The portrait
texture rotation matches the lab's `device-stage.js`.

The page provides two PNG images at 1600 × 2000:

- `#export`: cream background, saved here as `release/device.png`.
- `#transparent`: transparent background, saved here as `release/device-transparent.png`.

These supplement `release/cover.png`. The existing score-art cover and
versioned firmware package are unchanged.
