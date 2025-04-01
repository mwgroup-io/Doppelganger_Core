/*
 * Doppelgänger Pro v2 MWGroup Edition
 * Version 1.0.0 (22MAR2025)
 *
 * Professional RFID card cloning and analysis tool.
 * For authorized penetration testing use only.
 * Written by Travis Weathers (@tweathers-sec)
 */

#ifndef VERSION_CONFIG_H
#define VERSION_CONFIG_H

#include <Arduino.h>

// Version info
extern const char *version;
extern const char *builddate;
extern const char *device;
extern const char *hardware;

// System settings
#define CPU_FREQ_NORMAL 240 // MHz (only supported values are 240, 160, 80, 40)

// Debug output macros
extern bool DEBUG_ENABLED;

// Wiegand interface
#define MAX_BITS 100          // Max read bits
#define WEIGAND_WAIT_TIME 400 // ms between pulses (This is important for Keypad entries)
#define DATA0 16              // DATA0 pin
#define DATA1 15              // DATA1 pin

// Hardware GPIO pins
#define RST 33              // GPIO for hard reset (INPUT_PULLUP)
#define LED_ON HIGH         // LED on state
#define LED_OFF LOW         // LED off state
extern const int C_PIN_LED; // GPIO for the LED (OUTPUT)

// File paths
#define FORMAT_LITTLEFS_IF_FAILED true
#define CARDS_CSV_FILE "/www/cards.csv"
#define RESET_CARD_FILE "/www/reset_card.json"
#define NOTIFICATION_CONFIG_FILE "/notifications.json"

// WiFiManager Configurations
extern const char *defaultPASS;
extern const char *prefixSSID; // Will be defined in version_config.cpp
#define portalTimeout 180      // Device moves to the main loop after X seconds if no configuration is entered
#define connectTimeout 30      // Enters configuration mode if device can't find previously stored AP in X seconds

#endif // VERSION_CONFIG_H