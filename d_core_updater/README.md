# Doppelgänger Core - Firmware Builder

**Version 2.0.0** - Simplified Build System

This directory contains the updated firmware build and deployment system for the Doppelgänger Core device.

## Overview

This build system creates self-contained firmware installers for ESP32-S3 devices. It compiles firmware, embeds all necessary files, and produces cross-platform executables that can flash devices without external dependencies.

## Files

- `main.go` - Main firmware updater application (replaces template_main.go)
- `build_update_binaries.sh` - Updated build script with Windows support and verbose mode
- `current_hardware.txt` - Hardware build reference from PlatformIO

## System Requirements

### Development
- Go 1.19 or later
- PlatformIO Core
- ESP32 toolchain (auto-installed by PlatformIO)

### End Users
- esptool (auto-installed on macOS/Linux)
- USB drivers for ESP32 (usually auto-detected)

## Building

### Quick Build
```bash
./build_update_binaries.sh        # Auto-detects version from src/main.cpp
./build_update_binaries.sh -v     # Verbose mode with detailed output
```

### Cleanup Build Artifacts
```bash
./build_update_binaries.sh clean
```

### Help
```bash
./build_update_binaries.sh --help
```

### Manual Build Steps
1. Build firmware with PlatformIO:
   ```bash
   cd ..
   platformio run --environment esp32-s3-devkitc-1
   platformio run --target buildfs --environment esp32-s3-devkitc-1
   ```

2. Copy binaries:
   ```bash
   cd d_core_updater
   cp ../.pio/build/esp32-s3-devkitc-1/*.bin .
   cp ~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin .
   ```

3. Build updater for all platforms:
   ```bash
   go mod tidy
   GOOS=windows GOARCH=amd64 go build -o updater_windows_amd64.exe main.go
   GOOS=linux GOARCH=amd64 go build -o updater_linux_amd64 main.go
   GOOS=darwin GOARCH=amd64 go build -o updater_darwin_amd64 main.go
   # ... etc for other platforms
   ```

## Supported Platforms

### Build Platform
- macOS (Intel/Apple Silicon)
- Linux (x64/ARM/ARM64)
- Windows (x64/ARM64)

### Target Hardware
- ESP32-S3-DevKitC-1
- 8MB Flash, 320KB RAM
- USB-C connectivity

### Target Devices
- Core
- D_Core

## Usage

1. Connect ESP32 device via USB
2. Run the appropriate updater:
   - macOS: `./updater_darwin_amd64`
   - Linux: `./updater_linux_amd64` 
   - Windows: `updater_windows_amd64.exe`
3. Select the ESP32 port (auto-suggested)
4. Device will automatically reboot for validation
5. Wait for validation and flashing to complete

## Flash Memory Layout

Based on `current_hardware.txt` analysis:

| Component  | Address  | Size   | Description         |
| ---------- | -------- | ------ | ------------------- |
| Bootloader | 0x0000   | ~15KB  | ESP32-S3 bootloader |
| Partitions | 0x8000   | 3KB    | Partition table     |
| App0       | 0xe000   | 8KB    | Boot application    |
| Firmware   | 0x10000  | ~1.1MB | Main application    |
| LittleFS   | 0x670000 | ~1.5MB | Web interface files |

## Security Notes

- **No encryption keys required** - simplified deployment
- Device validation through serial console output only
- All binaries are stored in plain text
- Reduced attack surface compared to previous encrypted version

## Troubleshooting

### Build Issues
- Ensure Go 1.19+ is installed
- Run `go mod tidy` to resolve dependencies
- Check PlatformIO installation: `platformio --version`

### Flashing Issues
- Verify ESP32 drivers are installed
- Try different USB cables/ports
- Check esptool version (4.5.0+ required)
- Device reboot is triggered automatically

### Device Not Recognized
- Only "Core" and "D_Core" devices are supported
- Device must output identification strings via serial console
- Automatic reboot should trigger device identification
- If validation fails, try running the updater again

## Support

For technical support, contact: support@mayweathergroup.com

---
*Copyright (c) 2025 Mayweather Group, LLC*
