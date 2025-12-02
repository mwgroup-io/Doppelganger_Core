![Doppelganger_DEV_Board](https://store.physicalexploit.com/cdn/shop/files/DSC05101.jpg)

# Doppelgänger Core

![Doppelgänger Logo](https://github.com/mwgroup-io/Doppelganger_Core/blob/main/images/dop_logo.png)

Doppelgänger Core is a professional-grade RFID card cloning and analysis tool designed for authorized penetration testing. Built on the ESP32S3 platform, it offers advanced features for capturing and analyzing access control card data while maintaining operational security and ease of use.

## Current Version

- Version: 1.2.1
- Build Date: 02DEC2025

### Highlights
- Added Paxton/Net2 reader support with 75-bit protocol decoding
- Paxton KP75 keypad support with 55-56 bit keypad frame decoding
- HID/Paxton mode switching without device restart
- Paxton reset card support for WiFi credential clearing
- Enhanced CSV logging with card format identification
- Web UI for reader mode configuration
- Added AWID 50-bit card format support
  - Facility Code: 16 bits (bits 1–16)
  - Card Number: 32 bits (bits 17–48)
- Improved card data reporting and analysis

### Version 1.2.1 (02DEC2025)
- Fixed Paxton keypad press detection and decoding
- Improved Net2 frame processing with non-blocking timeout mechanism
- Enhanced notification support for all card types (HID and Paxton)
- Added email notifications for Paxton keypad presses
- Optimized interrupt handling for rapid keypad entry sequences
- Fixed card completion detection for 55-56 bit keypad frames

### Acknowledgments
Special thanks to [@00Waz](https://github.com/00Waz), [@jkramarz](https://github.com/jkramarz) and Dr0pR00t (Daniel Raines) for their work on the [Paxtogeddon](https://github.com/00Waz/Paxtogeddon-Reader) project, which was instrumental in understanding processing logic even after I spent thousands on purchasing Paxton controller, software, and fobs/cards. Additional thanks to [@en4rab](https://github.com/en4rab) for sharing additional Paxton resources with the community.

## Getting Started
To purchase the Doppelganger Longrange RFID Development Board head over to the [Practical Physical Exploitation Store](https://store.physicalexploit.com/). For detailed documentation on how to install, setup, and use the Doppelganger Longrange RFID Development Board visit the [Practical Physical Exploitation Product Documentation](https://physicalexploit.com/docs/products/rfid/doppelganger_development_board/).

## Hardware Specifications

### Development Board
* **Processor**: 
  * ESP32-S3-MINI-1-N8
  * Dual-core Xtensa® 32-bit LX7 CPU up to 240 MHz
  * Low-power co-processor for peripheral monitoring
* **Memory**:
  * 8MB Quad SPI Flash
  * No PSRAM (N8 variant)
  * Integrated flash supports >100,000 program/erase cycles
* **Operating Conditions**:
  * Ambient Temperature: -40°C ~ 85°C
  * Module Size: 15.4 × 20.5 × 2.4 mm

### Interfaces & I/O
* **Wiegand Interface**:
  * 2x Wiegand data pins with logic level conversion
  * Support for all standard Wiegand protocols
  * Automatic protocol detection (mixture of logic and guess work)
  * Error detection and filtering (mixture of logic and guess work)
* **Power**:
  * USB-C Power Input
  * 12V DC Input/Pass-through for RFID Reader
  * 3.3V and 5V Output Rails
* **Additional I/O**:
  * 2x Configurable GPIO Pins (GPIO35/36) for External Sensors
  * 1x Configurable Status LED
  * 1x Reset Button
  * 1x Boot Button
* **Wireless**:
  * WiFi 802.11 b/g/n (2.4GHz)
  * Bluetooth 5.0 (LE)

## Card Format Support

### Wiegand Formats
The table below summarizes various card data formats that Doppelganger supports. The bit positions shown represent the internal data portion (excluding any parity bits), and note that implementations may vary based on reader-specific processing.

| Card Type            | Format | Facility Code Bits | Card Number Bits    | Notes                                 |
| -------------------- | ------ | ------------------ | ------------------- | ------------------------------------- |
| HID H10301           | 26-bit | 8 (1–8)            | 16 (9–24)           | Standard Prox                         |
| Indala               | 26-bit | 8 (1–8)            | 16 (9–25)           | Requires Indala reader                |
| Indala               | 27-bit | 12 (1–12)          | 13 (14–26)          | Requires Indala reader                |
| 2804 WIEGAND         | 28-bit | 8 (4–11)           | 14 (13–26)          | Custom format                         |
| Indala               | 29-bit | 12 (1–12)          | 15 (14–28)          | Requires Indala reader                |
| ATS Wiegand          | 30-bit | 11 (2–12)          | 15 (14–28)          | Custom format                         |
| HID ADT              | 31-bit | 4 (1–4)            | 23 (5–27)           | ADT specific format                   |
| WEI32 (EM4102)       | 32-bit | 15 (1–15)          | 16 (16–31)          | EM4102 format                         |
| HID D10202           | 33-bit | 7 (1–7)            | 24 (8–31)           | Extended format                       |
| HID H10306           | 34-bit | 16 (1–16)          | 16 (17–32)          | Extended format                       |
| HID Corporate 1000   | 35-bit | 12 (2–13)          | 20 (14–33)          | Corporate format                      |
| HID Simplex (S12906) | 36-bit | 8 (1–8)            | 16 (19–34)          | Simplex format                        |
| HID H10304           | 37-bit | 16 (1–16)          | 19 (17–35)          | Extended format                       |
| HID H800002          | 46-bit | 14 (1–14)          | 30 (15–44)          | HID H800002 format                    |
| HID Corporate 1000   | 48-bit | 22 (2–23)          | 23 (24–46)          | Extended Corp format                  |
| AWID 50              | 50-bit | 16 (1–16)          | 32 (17–48)          | AWID extended format                  |
| Avigilon Avig56      | 56-bit | Configurable (N)   | Configurable (54−N) | 28/28 parity; site-configurable FC/CN |

### Paxton/Net2 Support
| Card Type   | Format | Notes                           |
| ----------- | ------ | ------------------------------- |
| Paxton Net2 | 75-bit | Net2 Fobs / Cards, EM Card Data |

### iCLASS Support
| Card Type     | Format   | Notes                                                             |
| ------------- | -------- | ----------------------------------------------------------------- |
| iCLASS Legacy | Standard | Legacy 2k/16k cards                                               |
| iCLASS SE     | Standard | Secure Element cards                                              |
| iCLASS Seos   | Standard | Latest generation secure cards                                    |
| PIV/MF Cards  | 32-bit   | UID extraction Only as that is what is provided in the datastream |

### Additional Features
* Keypad PIN Support
  * HID keypad: 4-bit Wiegand protocol
  * Paxton KP75 keypad: 55-56 bit Net2 protocol with automatic key detection
* Raw binary data capture
* HEX data conversion
* Automatic format detection
* Error detection and filtering
* Parity bit validation
* Site/facility code validation
* EMI and misread filtering

## Integration

### Supported RFID Readers
* HID MaxiProx 5375AGN00
* HID ICLASS SE R90 940NTNTEK00000 (with legacy support enabled)
* HID Indala ASR-620++ (No Longer Manufactured by HID)
* Paxton Net2 Proximity Readers (75-bit protocol)
* Paxton KP75 Keypads (55-56 bit keypad frame protocol)

### Required Components
| Item                                              | Qty |
| ------------------------------------------------- | --- |
| Doppelgänger RFID ESP32S3 Development Board       | 1   |
| USB-C Cable (for flashing firmare)                | 1   |
| 12V DC Power Supply (for RFID Reader)             | 1   |
| 5.5x2.1mm DC Power Cable                          | 1   |
| 22 AWG Solid Core Wire (Red, Black, Green, White) | 1   |
| Mounting Hardware Kit (Included)                  | 1   |

**Note:** Only attempt hardware integration if you are experienced with electronic assembly. Improper wiring may damage your equipment.

## Wiring Guide

> ⚠️ **POWER WARNING** ⚠️
> 
> - Only one 12V power source should be used at a time (either DC jack OR screw terminal)
> - USB-C and 12V power can be safely connected simultaneously
> - Always verify the correct polarity when using the screw terminal

### Power Connections
1. **12V DC Power Supply Options**
   - **Option 1: DC Power Jack**
     - Connect the 12V DC power supply to the 5.5x2.1mm DC power jack on the Doppelgänger board
   - **Option 2: 2-Pin Screw Terminal**
     - Alternatively, connect the 12V DC power supply to the included 2-pin screw terminal
   - The board will automatically regulate this down to 5V and 3.3V for internal use

2. **RFID Reader Power**
   - The Doppelgänger board provides a 12V DC pass-through connection for the RFID reader
   - Connect the reader's power input to the 12V DC output on the Doppelgänger board

### Wiegand Interface Connections
1. **Data Lines**
   - Connect the Green wire (Data0) from the RFID reader to the Data0 pin on the Doppelgänger board
   - Connect the White wire (Data1) from the RFID reader to the Data1 pin on the Doppelgänger board
   - Connect the Black wire (GND) from the RFID reader to any GND pin on the Doppelgänger board

2. **Power to RFID Reader**
   - Connect the Red wire (12V DC) from the Doppelgänger board to the RFID reader's power input
   - Connect the Black wire (GND) from the Doppelgänger board to the RFID reader's ground

### Connection Diagram
```
RFID Reader                     Doppelgänger Board
+----------------+             +------------------+
|                |             |                  |
| Data0 (Green)  |------------>| Data0            |
| Data1 (White)  |------------>| Data1            |
| GND (Black)    |------------>| GND              |
| 12V (Red)      |<------------| 12V DC Output    |
|                |             |                  |
+----------------+             +------------------+
```

### MaxiProx 5375

![HID MaxiProx 5375](https://physicalexploit.com/product-docs/maxiprox_wiring_d_dev.png)

### iCLASS R90SE

![HID iCLASS R90SE](https://physicalexploit.com/product-docs/r90se_wiring_d_dev.png)

**Important Safety Notes:**
- Always power off the device before making or changing connections
- Double-check all connections before applying power
- Ensure proper polarity when connecting power supplies
- Use appropriate wire gauges for power connections
- Keep wiring neat and organized to prevent shorts

## Getting Started

For detialed guidance on device usage, reference the [Product Documentation](https://physicalexploit.com/docs/products/code/doppelganger_core/).

1. Power on your Doppelgänger Pro device
2. Connect to the **doppelgänger_XXXX** network (default password: **UndertheRadar**)
3. Access the web interface at http://192.168.4.1 or http://rfid.local
4. Configure WiFi and notification settings
5. Begin capturing card data

[WiFi Setup](https://physicalexploit.com/product-docs/d_core_wifi_manager.png)


## Features

* **Real-time Notifications**: Email alerts, webhook support, web notifications
* **Professional Interface**: Modern web UI, real-time display, advanced exports
* **Detailed Logging**: Comprehensive debug information and card data analysis
* **Keypad Support**: HID 4-bit and Paxton 55-56 bit keypad capture with PIN sequence aggregation
* **GPIO**: Configurable GPIO for integrating haptic or other sensors

![Haptic Sensor](https://store.physicalexploit.com/cdn/shop/files/d_core_gpio_settings.jpg)

## Updating Firmware

There are three supported ways to update the firmware on Doppelgänger Core.

### Option 1: Use prebuilt installers from Releases (recommended)

1. Download the latest installer for your OS from the Releases page: [GitHub Releases](https://github.com/mwgroup-io/Doppelganger_Core/releases)
2. Connect your ESP32-S3 board via USB.
3. Run the downloaded installer for your platform (file name similar to `core_firmware_<version>_<os>_<arch>`):
   - macOS: make executable and run
     ```bash
     chmod +x ./core_firmware_<version>_darwin_<arch>
     ./core_firmware_<version>_darwin_<arch>
     ```
   - Linux: make executable and run
     ```bash
     chmod +x ./core_firmware_<version>_linux_<arch>
     ./core_firmware_<version>_linux_<arch>
     ```
   - Windows: run the `.exe` file
4. Select the suggested ESP32 serial port when prompted and follow on-screen instructions.

Notes:
- On macOS, you may need to allow execution (System Settings > Privacy & Security) if the binary was quarantined. Alternatively:
  ```bash
  xattr -d com.apple.quarantine ./core_firmware_<version>_darwin_<arch>
  ```
- USB drivers are typically not required for ESP32-S3; if needed, install the appropriate USB-to-serial driver for your OS.
- The updater automatically detects the product type and installs the appropriate firmware.

### Option 2: Build the updater locally (builder + flasher)

1. From the project root, run the updater build script:
   ```bash
   ./d_core_updater/build_binaries.sh
   # or with verbose logs
   ./d_core_updater/build_binaries.sh -v
   ```
2. After success, installers are located at:
   - `d_core_updater/build/installers_<version>_<timestamp>/`
3. Run the appropriate installer for your OS (same steps as in Option 1).

### Option 3: Manual update with PlatformIO (developer workflow)

1. Build the application and filesystem:
   ```bash
   platformio run --environment esp32-s3-devkitc-1
   platformio run --target buildfs --environment esp32-s3-devkitc-1
   ```
2. Upload the firmware and filesystem directly with PlatformIO:
   ```bash
   # Upload application
   platformio run --target upload --environment esp32-s3-devkitc-1
   # Upload filesystem (LittleFS)
   platformio run --target uploadfs --environment esp32-s3-devkitc-1
   ```
3. If you prefer using `esptool`, gather the generated binaries from `.pio/build/esp32-s3-devkitc-1/` and flash with these offsets:
   ```bash
   esptool.py --chip esp32s3 --port <port> --baud 460800 \
     --before default_reset --after hard_reset write_flash -z \
     --flash_mode qio --flash_freq 80m --flash_size 8MB \
     0x0000 bootloader.bin \
     0x8000 partitions.bin \
     0xE000 boot_app0.bin \
     0x10000 firmware.bin \
     0x670000 LittleFS.bin
   ```

Troubleshooting:
- Ensure the correct serial port is selected and not in use by another application.
- Try a different USB cable/port if the device is not detected.
- For persistent flashing issues, reduce baud to `115200` in `esptool.py` command.

## Legal Notice

This device is intended for professional penetration testing and authorized security assessments only. Unauthorized or illegal use/possession of this device is the sole responsibility of the user. Mayweather Group LLC, Practical Physical Exploitation, and the creator are not liable for illegal application of this device.

## License

[![License: CC BY-NC-ND 4.0](https://img.shields.io/badge/License-CC%20BY--NC--ND%204.0-lightgrey.svg)](https://creativecommons.org/licenses/by-nc-nd/4.0/)

This work is licensed under a [Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License](https://creativecommons.org/licenses/by-nc-nd/4.0/).

You are free to:
* Share — copy and redistribute the material in any medium or format

Under the following terms:
* Attribution — You must give appropriate credit, provide a link to the license, and indicate if changes were made.
* NonCommercial — You may not use the material for commercial purposes.
* NoDerivatives — If you remix, transform, or build upon the material, you may not distribute the modified material.

See the [LICENSE](LICENSE) file for the full license text.

## Support

For professional support and documentation, visit:
* [Practical Physical Exploitation Documentation](https://physicalexploit.com/docs/products/getting-started/)
* [GitHub Issues](https://github.com/tweathers-sec/doppelganger/issues)
* [Professional Store](https://store.physicalexploit.com/)

## Credits

Developed by Travis Weathers (@tweathers-sec)  
Copyright © 2025 Mayweather Group, LLC
