#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <Arduino.h>
#include "version_config.h"

class LEDManager
{
public:
    static LEDManager &getInstance();

    // Delete copy constructor and assignment operator
    LEDManager(const LEDManager &) = delete;
    LEDManager &operator=(const LEDManager &) = delete;

    // Initialize LED pin
    void begin();

    // Core LED operations - preserve existing behavior
    void turnOn();
    void turnOff();
    void toggle();

    // Startup sequence - preserve exact timing and pattern
    void runStartupSequence();

    // Reset button feedback sequences
    void runResetArmedSequence();
    void runResetSuccessSequence();
    void runResetTimeoutSequence();

    // Get current LED state
    bool isOn() const;

    // Update LED state - call this in main loop
    void update();

    // Utility methods
    void blink(int count, int onTime, int offTime);
    void pulse(int duration);

    // Hard reset sequences - preserve exact timing and pattern
    void signalResetArmed();
    void signalResetExecuted();
    void signalResetTimeout();

    // Get current LED state
    bool getCurrentState() const;

private:
    LEDManager() = default;
    ~LEDManager() = default;

    // Constants for LED states
    static constexpr uint8_t LED_ON_STATE = HIGH;
    static constexpr uint8_t LED_OFF_STATE = LOW;

    // LED pin - will be initialized in begin()
    int ledPin;

    bool currentState = false;

    // Timing control
    unsigned long lastStateChange = 0;
    unsigned long sequenceStartTime = 0;
    int sequenceStep = 0;
    bool inSequence = false;

    // Sequence timing constants
    static constexpr unsigned long STARTUP_INITIAL_DELAY = 2000;
    static constexpr unsigned long STARTUP_BLINK_DELAY = 100;
    static constexpr unsigned long STARTUP_PAUSE_DELAY = 500;
    static constexpr unsigned long STARTUP_FINAL_DELAY = 1000;
    static constexpr unsigned long RESET_ARMED_DELAY = 500;
    static constexpr unsigned long RESET_EXECUTED_DELAY = 200;
    static constexpr unsigned long RESET_TIMEOUT_DELAY = 1000;

    // Sequence state tracking
    int blinkCount = 0;
    unsigned long lastBlinkTime = 0;
};

#endif