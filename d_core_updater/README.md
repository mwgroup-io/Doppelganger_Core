# Doppelgänger Firmware Updater

**Version 1.1.0** - Universal Firmware Updater

This directory contains the firmware updater system for Doppelgänger devices. It creates self-contained executables that flash ESP32-S3 devices with embedded firmware files.

## Supported Products

- **Doppelgänger Stealth v2** - Professional RFID card cloning and analysis tool
- **Doppelgänger Core** - Advanced RFID card cloning with enhanced features

The updater automatically detects which product it's flashing based on the embedded firmware.

## Overview

The firmware updater is a single executable that contains all necessary firmware files embedded within it. It automatically detects ESP32-S3 devices, installs required tools (esptool), and flashes the complete firmware package. The updater dynamically detects whether it's flashing Stealth v2 or Core firmware and adjusts the interface accordingly.

## Files

### Source Code
- `src/main.go` - Main application entry point
- `src/flash.go` - Firmware flashing logic
- `src/serial.go` - Serial port detection and management
- `src/esptool.go` - ESPTool installation and management
- `src/utils.go` - Utility functions and constants

### Build System
- `build_binaries.sh` - Automated build script for all platforms

## System Requirements

### Development
- Go 1.19 or later
- PlatformIO Core
- ESP32 toolchain (auto-installed by PlatformIO)

### End Users
- No dependencies required - esptool is automatically installed
- USB drivers for ESP32-S3 (usually auto-detected)
- Compatible with both Stealth v2 and Core devices

## Building

### Quick Build
```bash
# From compiler directory:
./build_binaries.sh        # Build all platform executables
                           # Product type auto-detected from src/main.cpp
```

### Manual Build Steps
1. Build firmware with PlatformIO:
   ```bash
   cd ..
   platformio run --environment esp32-s3-devkitc-1
   platformio run --target buildfs --environment esp32-s3-devkitc-1
   ```

2. Build updater executables:
   ```bash
   cd compiler
   ./build_binaries.sh
   ```

## Supported Platforms

### Build Targets
- **macOS**: Intel (amd64) and Apple Silicon (arm64)
- **Linux**: x64 (amd64), ARM (arm), ARM64 (arm64)
- **Windows**: x64 (amd64), ARM64 (arm64)

### Target Hardware
- ESP32-S3-DevKitC-1
- 8MB Flash, 320KB RAM
- USB-C connectivity

## Usage

### Running the Updater
1. Connect ESP32-S3 device via USB
2. Run the appropriate executable:
   - **macOS**: `./stealth_firmware_1.1.0_darwin_arm64` or `./core_firmware_1.1.0_darwin_arm64`
   - **Linux**: `./stealth_firmware_1.1.0_linux_amd64` or `./core_firmware_1.1.0_linux_amd64`
   - **Windows**: `stealth_firmware_1.1.0_windows_amd64.exe` or `core_firmware_1.1.0_windows_amd64.exe`
   
   The updater automatically detects which product (Stealth v2 or Core) it's flashing.

### Command Line Options
- `-v` or `--verbose`: Enable detailed debugging output
- No arguments: Standard operation with essential output only

### Process Flow
1. **Tool Installation**: Automatically installs esptool if not present
2. **Device Detection**: Scans for and displays available serial ports
3. **Port Selection**: User selects ESP32 device (auto-suggested)
4. **Permission Check**: Verifies serial port access (Linux only)
5. **Firmware Flashing**: Extracts and flashes all firmware components
6. **Completion**: Displays success message and cleanup

## Flash Memory Layout

| Component  | Address  | Size   | Description         |
| ---------- | -------- | ------ | ------------------- |
| Bootloader | 0x0000   | ~15KB  | ESP32-S3 bootloader |
| Partitions | 0x8000   | 3KB    | Partition table     |
| App0       | 0xe000   | 8KB    | Boot application    |
| Firmware   | 0x10000  | ~1.1MB | Main application    |
| LittleFS   | 0x670000 | ~1.5MB | Web interface files |

## Features

### Automatic Tool Management
- Detects and installs esptool automatically on all platforms
- Uses appropriate package managers (Homebrew, apt, pip, winget)
- Handles both Python esptool.py and standalone esptool binaries

### Smart Device Detection
- Automatically identifies likely ESP32-S3 devices
- Shows detailed device information (VID/PID, descriptions)
- Prioritizes ESP32-specific USB devices
- Works with both Stealth v2 and Core hardware

### Robust Flashing
- Uses safe flash parameters (DIO/40MHz, 115200 baud, no-stub)
- Automatic retry with safer settings if initial flash fails
- Single-transaction flashing for consistency
- Real-time esptool output display

### Cross-Platform Compatibility
- Handles different esptool command syntax (Python vs standalone)
- Platform-specific serial port detection
- Automatic permission handling on Linux

## Output Examples

### Standard Mode
```
=== Doppelgänger Stealth v2 Firmware Updater ===

Available serial ports:
1: /dev/tty.usbmodem12201 (ESP32 Device - Native USB VID:303A PID:1001)

Recommended: Port 1 appears to be an ESP32 device
Select your ESP32 [1]: 

Flashing firmware...
Extracting firmware files...
[ESPTOOL] esptool.py v4.8.1
[ESPTOOL] Serial port /dev/tty.usbmodem12201
[ESPTOOL] Connecting...
[ESPTOOL] Chip is ESP32-S3 (QFN56) (revision v0.2)
...
✓ Firmware update completed successfully!
✓ Device should be running the new firmware.
Note: Please manually power cycle the device before use.
```

### Verbose Mode
```bash
./stealth_firmware_1.1.0_darwin_arm64 -v
# or
./core_firmware_1.1.0_darwin_arm64 -v
```
Includes additional debugging information and command execution details.

## Troubleshooting

### Build Issues
- Ensure Go 1.19+ is installed: `go version`
- Check PlatformIO installation: `platformio --version`
- Verify binary files are present in build artifacts

### Flashing Issues
- Try different USB cables/ports
- Ensure device is in normal mode (not download mode)
- Check for conflicting serial applications
- Use verbose mode (`-v`) for detailed diagnostics

### Permission Issues (Linux)
- User must be in `dialout` group
- Run `newgrp dialout` or log out/in after group addition
- Check port permissions: `ls -la /dev/ttyUSB* /dev/ttyACM*`

### Device Not Detected
- Verify USB drivers are installed
- Check device manager (Windows) or system information
- Try different USB ports
- Ensure ESP32-S3 device is powered and responsive
- Compatible with both Stealth v2 and Core hardware

## Technical Details

### Embedded Files
All firmware components are embedded directly in the executable using Go's `embed` directive:
- bootloader.bin
- partitions.bin  
- boot_app0.bin
- firmware.bin
- LittleFS.bin

### Flash Parameters
- **Mode**: DIO (Dual I/O)
- **Frequency**: 40MHz
- **Baud Rate**: 115200
- **Flash Size**: 8MB
- **Options**: --no-stub, -z (compressed)

## Product Information

### Doppelgänger Stealth v2
Professional RFID card cloning and analysis tool for authorized penetration testing.

### Doppelgänger Core  
Advanced RFID card cloning with enhanced features and improved performance.

Both products use the same ESP32-S3 platform and flashing process. The updater automatically detects which firmware is being installed.

## Support

For technical support, contact: support@mayweathergroup.com

---
*Copyright (c) 2025 Mayweather Group, LLC*