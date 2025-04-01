#include "led_manager.h"
#include "logger.h"

LEDManager &LEDManager::getInstance()
{
    static LEDManager instance;
    return instance;
}

void LEDManager::begin()
{
    ledPin = C_PIN_LED; // Initialize pin from version_config.h
    pinMode(ledPin, OUTPUT);
    turnOn(); // Initial state as per original
}

void LEDManager::turnOn()
{
    digitalWrite(ledPin, LED_ON_STATE);
    currentState = true;
}

void LEDManager::turnOff()
{
    digitalWrite(ledPin, LED_OFF_STATE);
    currentState = false;
}

void LEDManager::toggle()
{
    digitalWrite(ledPin, digitalRead(ledPin) == LED_ON_STATE ? LED_OFF_STATE : LED_ON_STATE);
    currentState = !currentState;
}

bool LEDManager::isOn() const
{
    return currentState;
}

bool LEDManager::getCurrentState() const
{
    return currentState;
}

void LEDManager::runStartupSequence()
{
    // Initial state - LED off
    digitalWrite(C_PIN_LED, LED_OFF);
    delay(2000);

    // Three quick blinks
    for (int i = 0; i < 3; i++)
    {
        digitalWrite(C_PIN_LED, LED_ON);
        delay(100);
        digitalWrite(C_PIN_LED, LED_OFF);
        delay(100);
    }

    delay(500);

    // Final state - LED on for 1 second then off
    digitalWrite(C_PIN_LED, LED_ON);
    delay(1000);
    digitalWrite(C_PIN_LED, LED_OFF);
}

void LEDManager::signalResetArmed()
{
    sequenceStartTime = millis();
    sequenceStep = 0;
    inSequence = true;
    turnOn(); // Start with LED on
    logger.logLEDStatus("[LED] Reset armed - waiting for confirmation");
}

void LEDManager::signalResetExecuted()
{
    sequenceStartTime = millis();
    sequenceStep = 0;
    inSequence = true;
    turnOn(); // Start with LED on
    logger.logLEDStatus("[LED] Reset successful - restarting device");
}

void LEDManager::signalResetTimeout()
{
    sequenceStartTime = millis();
    sequenceStep = 0;
    inSequence = true;
    turnOn(); // Start with LED on
    logger.logLEDStatus("[LED] Reset timeout - exiting armed mode");
}

void LEDManager::update()
{
    if (!inSequence)
        return;

    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - sequenceStartTime;

    // Startup sequence
    if (sequenceStep == 0 && elapsedTime >= STARTUP_INITIAL_DELAY)
    {
        // Three quick blinks
        if (blinkCount < 3)
        {
            if (elapsedTime - lastBlinkTime >= STARTUP_BLINK_DELAY)
            {
                if (currentState)
                {
                    turnOff();
                }
                else
                {
                    turnOn();
                    blinkCount++;
                }
                lastBlinkTime = currentTime;
            }
        }
        else
        {
            sequenceStep = 1;
            sequenceStartTime = currentTime;
            blinkCount = 0;
        }
    }
    else if (sequenceStep == 1 && elapsedTime >= STARTUP_PAUSE_DELAY)
    {
        // Final long blink
        if (!currentState)
        {
            turnOn();
        }
        else if (elapsedTime - lastBlinkTime >= STARTUP_FINAL_DELAY)
        {
            turnOff();
            inSequence = false;
        }
    }

    // Reset armed sequence
    else if (sequenceStep == 0 && elapsedTime >= RESET_ARMED_DELAY)
    {
        if (blinkCount < 2)
        {
            if (elapsedTime - lastBlinkTime >= RESET_ARMED_DELAY)
            {
                if (currentState)
                {
                    turnOff();
                }
                else
                {
                    turnOn();
                    blinkCount++;
                }
                lastBlinkTime = currentTime;
            }
        }
        else
        {
            inSequence = false;
            blinkCount = 0;
        }
    }

    // Reset executed sequence
    else if (sequenceStep == 0 && elapsedTime >= RESET_EXECUTED_DELAY)
    {
        if (blinkCount < 5)
        {
            if (elapsedTime - lastBlinkTime >= RESET_EXECUTED_DELAY)
            {
                if (currentState)
                {
                    turnOff();
                }
                else
                {
                    turnOn();
                    blinkCount++;
                }
                lastBlinkTime = currentTime;
            }
        }
        else
        {
            inSequence = false;
            blinkCount = 0;
        }
    }

    // Reset timeout sequence
    else if (sequenceStep == 0 && elapsedTime >= RESET_TIMEOUT_DELAY)
    {
        turnOff();
        inSequence = false;
    }
}

void LEDManager::blink(int count, int onTime, int offTime)
{
    static unsigned long lastBlinkTime = 0;
    static int currentBlinkCount = 0;
    static bool isOn = false;
    static unsigned long currentTime = 0;

    currentTime = millis();

    if (currentBlinkCount < count)
    {
        if (isOn)
        {
            if (currentTime - lastBlinkTime >= onTime)
            {
                turnOff();
                isOn = false;
                lastBlinkTime = currentTime;
            }
        }
        else
        {
            if (currentTime - lastBlinkTime >= offTime)
            {
                turnOn();
                isOn = true;
                currentBlinkCount++;
                lastBlinkTime = currentTime;
            }
        }
    }
}

void LEDManager::pulse(int duration)
{
    static unsigned long pulseStartTime = 0;
    static bool pulseActive = false;
    static unsigned long currentTime = 0;

    currentTime = millis();

    if (!pulseActive)
    {
        turnOn();
        pulseStartTime = currentTime;
        pulseActive = true;
    }
    else if (currentTime - pulseStartTime >= duration)
    {
        turnOff();
        pulseActive = false;
    }
}