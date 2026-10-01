# Score and sample credits

Rill Chamber firmware code is © 2026 Bruce Blay, GPL-3.0-or-later.
The embedded recordings and score editions retain the licenses below.

## Score provenance

The committed score arrays were exported from [Rill Sound](https://github.com/bruceblay/rill-sound)
by `tools/export_scores.mjs`. The source catalog consulted for this release is
at commit `630346051b8f425509f369b80ff925143cf15347`. Normal firmware builds use the generated arrays in
`src/Pieces.h`; they do not download or regenerate scores.

The five process studies use original figures by Bruce Blay with processes
after Steve Reich, Philip Glass and Frederic Rzewski. They are studies from the
Rill Sound lab, not transcriptions of the composers' published scores.

The 45 Bach works use the MIDI editions below. The import converts MIDI ticks
to beat positions, extracts voices, and stores pitch, start and duration as
C++ arrays. Shared staves are split into voices by pitch (Sinfonias and
chorales) or the canon relationship (Goldberg Variation 3). Tempo and playback
instrument are set by the lab. The augmentation edition's written-out
ornaments are retained.

Data derived from the five CC BY-SA editions (Inventions 6, 9 and 12,
Goldberg Variation 3 and Canon by Augmentation), including their generated
note arrays and score vectors, remains under
[CC BY-SA 3.0 Unported](https://creativecommons.org/licenses/by-sa/3.0/).
Changes are the conversions and voice assignments described above. The code
license does not replace these score licenses. Source links identify the
original editions; no endorsement by their contributors is implied.

| Piece | Edition / attribution | License |
| --- | --- | --- |
| Invention No. 1 (BWV 772) | [Mutopia Project (via Wikimedia Commons)](https://commons.wikimedia.org/wiki/File:Bach_-_Invention_01.mid) | Public domain |
| Invention No. 2 (BWV 773) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=58) | Public domain |
| Invention No. 3 (BWV 774) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=70) | Public domain |
| Invention No. 4 (BWV 775) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=67) | Public domain |
| Invention No. 5 (BWV 776) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=55) | Public domain |
| Invention No. 6 (BWV 777) | [jeff covey / Mutopia Project](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=159) | CC BY-SA 3.0 |
| Invention No. 7 (BWV 778) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=73) | Public domain |
| Invention No. 8 (BWV 779) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=61) | Public domain |
| Invention No. 9 (BWV 780) | [jeff covey / Mutopia Project](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=171) | CC BY-SA 3.0 |
| Invention No. 10 (BWV 781) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=62) | Public domain |
| Invention No. 11 (BWV 782) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=71) | Public domain |
| Invention No. 12 (BWV 783) | [jeff covey / Mutopia Project](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=174) | CC BY-SA 3.0 |
| Invention No. 13 (BWV 784) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=59) | Public domain |
| Invention No. 14 (BWV 785) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=72) | Public domain |
| Invention No. 15 (BWV 786) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=68) | Public domain |
| Sinfonia No. 1 (BWV 787) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=142) | Public domain |
| Sinfonia No. 2 (BWV 788) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=140) | Public domain |
| Sinfonia No. 3 (BWV 789) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=143) | Public domain |
| Sinfonia No. 4 (BWV 790) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=172) | Public domain |
| Sinfonia No. 5 (BWV 791) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=204) | Public domain |
| Sinfonia No. 6 (BWV 792) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=173) | Public domain |
| Sinfonia No. 7 (BWV 793) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=175) | Public domain |
| Sinfonia No. 8 (BWV 794) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=176) | Public domain |
| Sinfonia No. 9 (BWV 795) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=199) | Public domain |
| Sinfonia No. 10 (BWV 796) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=205) | Public domain |
| Sinfonia No. 11 (BWV 797) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=200) | Public domain |
| Sinfonia No. 12 (BWV 798) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=144) | Public domain |
| Sinfonia No. 13 (BWV 799) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=206) | Public domain |
| Sinfonia No. 14 (BWV 800) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=213) | Public domain |
| Sinfonia No. 15 (BWV 801) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=145) | Public domain |
| Contrapunctus I (BWV 1080) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=693) | Public domain |
| Contrapunctus II (BWV 1080) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=719) | Public domain |
| Contrapunctus III (BWV 1080) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=720) | Public domain |
| Contrapunctus IV (BWV 1080) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=721) | Public domain |
| Contrapunctus V (BWV 1080) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=722) | Public domain |
| Contrapunctus VI (BWV 1080) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=734) | Public domain |
| Contrapunctus VII (BWV 1080) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=736) | Public domain |
| Contrapunctus VIII (BWV 1080) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=739) | Public domain |
| Contrapunctus IX (BWV 1080) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=740) | Public domain |
| Contrapunctus X (BWV 1080) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=744) | Public domain |
| Goldberg Variation 3 (BWV 988) | [Hajo Dezelski / Mutopia Project](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=1384) | CC BY-SA 3.0 |
| Canon by Augmentation (BWV 1079) | [AugPi, © 2005 / Wikimedia Commons](https://commons.wikimedia.org/wiki/File:J_S_Bach_--_Canon_per_augmentationem_contrario_motu_(dulcimer-piano-harp)_(ornam).mid) | CC BY-SA 3.0 |
| Ricercar a 6 (BWV 1079) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=702) | Public domain |
| Aus meines Herzens Grunde (BWV 269) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=378) | Public domain |
| Ich dank dir, lieber Herre (BWV 347) | [Mutopia Project contributors](https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=379) | Public domain |

## Recordings

- **Piano:** [Versilian Community Sample Library (VCSL)](https://github.com/sgossner/VCSL),
  by Sam Gossner and VCSL contributors, [CC0](https://creativecommons.org/publicdomain/zero/1.0/).
  Kawai Grand Piano / Sustains / v2, eight zones, MIDI 46–86. The selection
  comes from Rill Mallet; Chamber's exporter resamples/embeds it for 32 kHz playback.
- **Hand claps:** “Palmas sevillanas (flamenco clapping), 160 BPM” by
  Santiago Sánchez Cifuentes, [Wikimedia Commons source](https://commons.wikimedia.org/wiki/File:Palmas_sevillanas_(flamenco_clapping),_160_BPM.ogg),
  CC0. Two 150 ms excerpts at 56.6 s and 96.0 s, with a 10 ms fade and peak
  normalization in Rill Sound, then converted to 32 kHz PCM for Chamber.
- Harpsichord, marimba, xylophone, glass, strings and organ are synthesized
  by the firmware; they are not additional third-party recordings.

`tools/embed_samples.py` records the conversion into `src/Samples.h`.

## Firmware dependencies

- [M5Unified 0.2.21](https://github.com/m5stack/M5Unified/tree/0.2.21): MIT.
- [M5GFX 0.2.28](https://github.com/m5stack/M5GFX/tree/0.2.28): MIT; includes
  components with their own notices in that project's source.
- [Arduino ESP32 2.0.17](https://github.com/espressif/arduino-esp32/tree/2.0.17):
  LGPL-2.1 and bundled component licenses; see the upstream source notices.
- [Espressif32 PlatformIO platform 6.12.0](https://github.com/platformio/platform-espressif32/tree/v6.12.0)
  pins the framework and toolchain used to build this release.

Dependency sources are fetched by PlatformIO using the versions in
`platformio.ini`. The release package includes this repository's complete
source archive and the build instructions needed to rebuild it.
