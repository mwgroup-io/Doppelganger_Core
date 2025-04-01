#ifndef GPIO_MANAGER_H
#define GPIO_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>
#include <driver/gpio.h>
#include "card_processor.h"

// Forward declaration of CardProcessor
class CardProcessor;
extern CardProcessor cardProcessor;

// GPIO Pin Definitions
#define GPIO_PIN_35 GPIO_NUM_35
#define GPIO_PIN_36 GPIO_NUM_36

class GPIOManager
{
private:
    static GPIOManager *instance;
    bool initialized;
    bool pin35Enabled;
    bool pin36Enabled;
    bool pin35DefaultHigh;  // Default state when enabled
    bool pin36DefaultHigh;  // Default state when enabled
    int pin35PulseDuration; // Duration in milliseconds for pin 35
    int pin36PulseDuration; // Duration in milliseconds for pin 36

    // Timer variables for non-blocking pulses
    unsigned long pin35PulseStartTime;
    unsigned long pin36PulseStartTime;
    bool pin35Pulsing;
    bool pin36Pulsing;

    const char *CONFIG_FILE = "/www/gpio_settings.json";

    GPIOManager();
    bool loadConfig();
    bool saveConfig();
    void initializeGPIO(); // New method for GPIO initialization

public:
    static GPIOManager &getInstance();
    void begin();
    void loop();

    // Enable/disable pins
    void setPin35Enabled(bool enabled, bool defaultHigh = false);
    void setPin36Enabled(bool enabled, bool defaultHigh = false);
    bool isPin35Enabled() const;
    bool isPin36Enabled() const;

    // Get current states
    bool getPin35DefaultHigh() const;
    bool getPin36DefaultHigh() const;

    // Pulse duration
    void setPin35PulseDuration(int duration);
    void setPin36PulseDuration(int duration);
    int getPin35PulseDuration() const;
    int getPin36PulseDuration() const;

    // Handle card read events
    void pulsePin35();
    void pulsePin36();

    void resetToDefaults();
    bool isInitialized() const;
};

#endif // GPIO_MANAGER_H