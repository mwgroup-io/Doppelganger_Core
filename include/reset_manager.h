#pragma once

#include <Arduino.h>
#include "version_config.h" // For RST pin definition
#include "led_manager.h"
#include "wifi_setup_manager.h"

class ResetManager
{
public:
    static ResetManager &getInstance();
    void begin();
    void update();

private:
    ResetManager() = default;
    static ResetManager instance;

    static const unsigned long DEBOUNCE_DELAY = 500;
    static const unsigned long RESET_TIMEOUT = 5000;

    unsigned long firstPressTime = 0;
    bool waitingForSecondPress = false;

    void handleReset();
};