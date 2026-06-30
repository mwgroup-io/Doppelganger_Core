# Doppelganger Core - Claude project guide

ESP32-S3 RFID access-control card capture/analysis tool for authorized physical-security
penetration testing (a PPE product, v1.2.3). Taps an access-control reader's Wiegand bus,
decodes presented credentials + keypad PINs, logs them, and streams them in real time to the
ATLAS platform. Global rules in `~/.claude/CLAUDE.md` apply.

## What it does
- Sits inline on the reader-to-controller **Wiegand D0/D1** lines and decodes presented
  credentials (and keypad PIN entry) as a pass-through tap.
- Stores reads in LittleFS, serves a web UI (WiFiManager AP or joined WiFi), sends SMTP
  notifications, and streams reads to ATLAS over WebSocket (GPS-tagged on the ATLAS side).

## Architecture (manager singletons; `main.cpp` only wires them)
| Module (`src/`,`include/`) | Responsibility |
|------|------|
| `wiegand_interface` | ISR capture of D0/D1 pulses into a bit buffer |
| `card_processor` (~27k, the core) | decode Wiegand bits -> card data; keypad handling |
| `card_event_handler` | dispatch card/keypad events to log/stream/notify |
| `reader_manager` | reader lifecycle/config |
| `net2_interface` | Paxton Net2 access-control integration |
| `websocket_handler` (~21k) | real-time card streaming (ATLAS) |
| `wifi_setup_manager` | WiFiManager AP setup, join, NTP time |
| `email_manager` | SMTP notifications |
| `gpio_manager` | aux outputs on GPIO 35 / 36 (configurable enable + default level) |
| `keypad_processor`, `led_manager`, `notification_manager` | keypad, LEDs, notifications |
| `logger`, `debug_manager` | LittleFS logging + debug |
| `reset_manager`, `reset_card_manager` | factory reset + a field "reset card" |
| `version_manager` / `version_config.h` | version + **all pin definitions** |

## Directory map
| Path | What |
|------|------|
| `src/`, `include/` | firmware (one .cpp/.h per manager) |
| `include/version_config.h` | version + PIN definitions (Wiegand D0/D1, keypad, LEDs) - read for exact GPIOs |
| `data/www/` | web UI (served from LittleFS) |
| `d_core_updater/` | host-side firmware updater |
| `lib/`, `test/` | libraries + tests |
| `platformio.ini` | build config |

## Build
```
pio run                 # build (env esp32-s3-devkitc-1)
pio run -t upload       # flash
pio run -t uploadfs     # upload data/www to LittleFS
```
Board: ESP32-S3 DevKitC-1, 8MB flash, LittleFS, no PSRAM, USB-CDC-on-boot (serial 115200).
Libs: ESPAsyncWebServer, ArduinoJson, arduinoWebSockets, WiFiManager.

## Pins / hardware
- Wiegand D0/D1, keypad, LED pins: in `include/version_config.h` (authoritative).
- GPIO 35 / GPIO 36: software-controlled aux outputs (`gpio_manager.cpp`) - enable + default
  level configurable (e.g. relay/aux trigger).

## Web / API
- Config + monitoring UI served from `data/www/` (AP `Doppelganger_XXXX` or joined network;
  reachable via mDNS/hostname). Reads pushed over WebSocket; ATLAS connectivity + network-info
  endpoints for streaming. (Routes are registered in `websocket_handler` / `wifi_setup_manager`.)

## ATLAS integration
Streams captured reads to ATLAS (`atlas.mwgroup.io`) over WiFi/WebSocket for GPS-tagged,
project-managed reporting. Keep the streaming contract compatible with ATLAS.

## Relationship to d_stealth_v2
`d_stealth_v2` is the covert/stealth sibling firmware. Core is the full-featured dev-board
build (web UI, email, keypad, LEDs).

## Conventions
camelCase functions/vars, PascalCase manager classes, snake_case filenames. One manager per
concern; `main.cpp` only initializes + wires them. No emoji in code, UI, logs, or docs.
