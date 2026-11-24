# Vinder 1.8

## Appearance presets

- Choose **Follow System**, **Light**, or **Dark** from Interface settings. Preview tiles show each appearance before selection.
- Blur controls use debounced sliders, with menu blur presets and offsets based on actual window sizes for consistent results.

## Responsive thumbnail preparation

- Background thumbnail preparation waits for user inactivity, pauses in power saver mode, and watches configured folders.
- Fixed input handling so the activity monitor never consumes clicks or key presses. Added a regression test for event propagation.

## Build and packaging

- CI builds and tests the Vinder, Devel, and stock profiles, then builds and smoke-tests the Flatpak bundle.
- Flatpak releases include a SHA256 checksum and GitHub build provenance attestation.
