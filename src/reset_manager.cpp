#include "reset_manager.h"

ResetManager ResetManager::instance;

ResetManager &ResetManager::getInstance()
{
    return instance;
}

void ResetManager::begin()
{
    pinMode(RST, INPUT_PULLUP);
}

void ResetManager::update()
{
    if (digitalRead(RST) == LOW)
    {
        if (!waitingForSecondPress && (millis() - firstPressTime >= DEBOUNCE_DELAY))
        {
            firstPressTime = millis();
            waitingForSecondPress = true;
            Serial.println("Entering Hard Reset arming mode. Waiting for second press within 5 seconds...");
            LEDManager::getInstance().signalResetArmed();
        }
        else if (waitingForSecondPress && (millis() - firstPressTime >= DEBOUNCE_DELAY))
        {
            handleReset();
        }
    }
    else
    {
        // Check for reset timeout
        if (waitingForSecondPress && (millis() - firstPressTime >= RESET_TIMEOUT))
        {
            Serial.println("Hard reset timed out. Exiting arming mode.");
            waitingForSecondPress = false;
            firstPressTime = 0;
            LEDManager::getInstance().signalResetTimeout();
        }
    }
}

void ResetManager::handleReset()
{
    Serial.println("Executing hard reset...");
    waitingForSecondPress = false;
    firstPressTime = 0;
    LEDManager::getInstance().signalResetExecuted();
    delay(2000);
    wifiSetupManager.resetStoredWiFi();
}