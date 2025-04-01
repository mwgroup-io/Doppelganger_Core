# Doppelgänger Pro v2

![Doppelgänger Logo](https://github.com/optiv/doppelganger/blob/main/git_images/dop_logo.png)

Doppelgänger Pro v2 is a professional-grade RFID card cloning and analysis tool designed for authorized penetration testing. Built on the ESP32S3 platform, it offers advanced features for capturing and analyzing access control card data while maintaining operational security and ease of use.

## Getting Started
To purchase the Doppelganger Longrange RFID Development Board head over to the [Practical Physical Exploitation Store](https://store.physicalexploit.com/). For detailed documentation on how to install, setup, and use the Doppelganger Longrange RFID Development Board visit the [Practical Physical Exploitation Store](https://physicalexploit.com/docs/products/getting-started/).

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

| Card Type            | Format | Facility Code Bits | Card Number Bits | Notes                  |
| -------------------- | ------ | ------------------ | ---------------- | ---------------------- |
| HID H10301           | 26-bit | 8 (1–8)            | 16 (9–24)        | Standard Prox          |
| Indala               | 26-bit | 8 (1–8)            | 16 (9–25)        | Requires Indala reader |
| Indala               | 27-bit | 12 (1–12)          | 13 (14–26)       | Requires Indala reader |
| 2804 WIEGAND         | 28-bit | 8 (4–11)           | 14 (13–26)       | Custom format          |
| Indala               | 29-bit | 12 (1–12)          | 15 (14–28)       | Requires Indala reader |
| ATS Wiegand          | 30-bit | 11 (2–12)          | 15 (14–28)       | Custom format          |
| HID ADT              | 31-bit | 4 (1–4)            | 23 (5–27)        | ADT specific format    |
| WEI32 (EM4102)       | 32-bit | 15 (1–15)          | 16 (16–31)       | EM4102 format          |
| HID D10202           | 33-bit | 7 (1–7)            | 24 (8–31)        | Extended format        |
| HID H10306           | 34-bit | 16 (1–16)          | 16 (17–32)       | Extended format        |
| HID Corporate 1000   | 35-bit | 12 (2–13)          | 20 (14–33)       | Corporate format       |
| HID Simplex (S12906) | 36-bit | 8 (1–8)            | 16 (19–34)       | Simplex format         |
| HID H10304           | 37-bit | 16 (1–16)          | 19 (17–35)       | Extended format        |
| HID Corporate 1000   | 48-bit | 22 (2–23)          | 23 (24–46)       | Extended Corp format   |

### iCLASS Support
| Card Type     | Format   | Notes                                                            |
| ------------- | -------- | ---------------------------------------------------------------- |
| iCLASS Legacy | Standard | Legacy 2k/16k cards                                              |
| iCLASS SE     | Standard | Secure Element cards                                             |
| iCLASS Seos   | Standard | Latest generation secure cards                                   |
| PIV/MF Cards  | 32-bit   | UID extraction Only as that is what is provided in the datasteam |

### Additional Features
* Keypad PIN Support (4-bit)
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

### Required Components
| Item                                        | Qty |
| ------------------------------------------- | --- |
| Doppelgänger RFID ESP32S3 Development Board | 1   |
| USB-C Power Supply                          | 1   |
| 12V DC Power Supply (for RFID Reader)       | 1   |
| 5.5x2.1mm DC Power Cable                    | 1   |
| 22 AWG Solid Core Wire (Various Colors)     | 1   |
| Mounting Hardware Kit                       | 1   |

**Note:** Only attempt hardware integration if you are experienced with electronic assembly. Improper wiring may damage your equipment.

## Getting Started

1. Power on your Doppelgänger Pro device
2. Connect to the **doppelgänger_XXXX** network (default password: **UndertheRadar**)
3. Access the web interface at http://192.168.4.1 or http://rfid.local
4. Configure WiFi and notification settings
5. Begin capturing card data

## Features

* **Real-time Notifications**: Email alerts, webhook support, web notifications
* **Enhanced Security**: Secure boot, flash encryption, OTA protection
* **Professional Interface**: Modern web UI, real-time display, advanced exports
* **Detailed Logging**: Comprehensive debug information and card data analysis

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