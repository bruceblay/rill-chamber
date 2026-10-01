# Changelog

## 0.1.0 — release candidate, 2026-10-01

First M5Burner release preparation for Rill Chamber.

- 50 pieces across seven collections, including five process studies and 45 Bach pieces.
- One to four StickS3s share a score over ESP-NOW; a solo device plays all parts.
- Shared play, stop and piece selection; local volume with a low-battery cap.
- Apply stop proposals to the audio engine before updating the playback flag.
- Copy serial audio diagnostics under a lock to avoid cross-task reads.
- Reject foreign protocol packets and invalid control states before they affect the ensemble.
- Keep display snapshots tied to their score when changing pieces.
- Embed the version in the startup log and provide a factory-image packager.
- Bundle score/sample attribution, a store description and a release cover.

Prepared for release; store submission and hardware acceptance are not recorded yet.
