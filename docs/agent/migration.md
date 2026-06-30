# Migration manifest — gitignored build inputs

Doppelganger Core is **PlatformIO firmware** (ESP32). A fresh clone builds with
`pio run`; PlatformIO downloads the platform toolchain and the `lib_deps` from
`platformio.ini`. **There are no gitignored build inputs to migrate.**

## Regenerated/downloaded — no action
- `.pio/` — PlatformIO build dir + downloaded platform/libraries.

## Host toolchain
- PlatformIO Core (`pio`); flash with `pio run -t upload`.
