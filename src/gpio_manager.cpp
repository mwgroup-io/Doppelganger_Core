#include "gpio_manager.h"

GPIOManager *GPIOManager::instance = nullptr;

GPIOManager::GPIOManager() : initialized(false),
                             pin35Enabled(false), pin36Enabled(false),
                             pin35DefaultHigh(false), pin36DefaultHigh(false),
                             pin35PulseDuration(1000), pin36PulseDuration(1000), // Default 1 second pulse
                             pin35PulseStartTime(0), pin36PulseStartTime(0),
                             pin35Pulsing(false), pin36Pulsing(false)
{
    initializeGPIO();
}

void GPIOManager::initializeGPIO()
{
    // Configure GPIO pins
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << GPIO_PIN_35) | (1ULL << GPIO_PIN_36);
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    // Set initial states
    gpio_set_level(GPIO_PIN_35, 0);
    gpio_set_level(GPIO_PIN_36, 0);
}

GPIOManager &GPIOManager::getInstance()
{
    if (instance == nullptr)
    {
        instance = new GPIOManager();
    }
    return *instance;
}

void GPIOManager::begin()
{
    if (!initialized)
    {
        // Initialize GPIO hardware first
        initializeGPIO();

        // Load and apply saved settings
        if (!loadConfig())
        {
            // If loading fails, apply safe defaults
            pin35Enabled = false;
            pin36Enabled = false;
            pin35DefaultHigh = false;
            pin36DefaultHigh = false;
            pin35PulseDuration = 1000;
            pin36PulseDuration = 1000;

            // Save these defaults
            saveConfig();
        }

        initialized = true;
    }
}

bool GPIOManager::loadConfig()
{
    if (!LittleFS.exists(CONFIG_FILE))
    {
        // Create default config file if it doesn't exist
        JsonDocument doc;
        doc["pin35_enabled"] = false;
        doc["pin36_enabled"] = false;
        doc["pin35_default_high"] = false;
        doc["pin36_default_high"] = false;
        doc["pin35_pulse_duration"] = 1000;
        doc["pin36_pulse_duration"] = 1000;

        File file = LittleFS.open(CONFIG_FILE, "w");
        if (!file || serializeJson(doc, file) == 0)
        {
            if (file)
                file.close();
            return false;
        }
        file.close();
    }

    File file = LittleFS.open(CONFIG_FILE, "r");
    if (!file)
    {
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error)
    {
        return false;
    }

    // Load settings from file
    pin35Enabled = doc["pin35_enabled"] | false;
    pin36Enabled = doc["pin36_enabled"] | false;
    pin35DefaultHigh = doc["pin35_default_high"] | false;
    pin36DefaultHigh = doc["pin36_default_high"] | false;
    pin35PulseDuration = doc["pin35_pulse_duration"] | 1000;
    pin36PulseDuration = doc["pin36_pulse_duration"] | 1000;

    // Apply loaded settings to the card processor
    cardProcessor.setPin35OnCardRead(pin35Enabled);
    cardProcessor.setPin36OnCardRead(pin36Enabled);

    // Apply loaded settings to GPIO pins
    gpio_set_level(GPIO_PIN_35, pin35Enabled && pin35DefaultHigh ? 1 : 0);
    gpio_set_level(GPIO_PIN_36, pin36Enabled && pin36DefaultHigh ? 1 : 0);

    return true;
}

bool GPIOManager::saveConfig()
{
    JsonDocument doc;
    doc["pin35_enabled"] = pin35Enabled;
    doc["pin36_enabled"] = pin36Enabled;
    doc["pin35_default_high"] = pin35DefaultHigh;
    doc["pin36_default_high"] = pin36DefaultHigh;
    doc["pin35_pulse_duration"] = pin35PulseDuration;
    doc["pin36_pulse_duration"] = pin36PulseDuration;

    File file = LittleFS.open(CONFIG_FILE, "w");
    if (!file)
    {
        return false;
    }

    if (serializeJson(doc, file) == 0)
    {
        file.close();
        return false;
    }

    file.close();
    return true;
}

void GPIOManager::setPin35Enabled(bool enabled, bool defaultHigh)
{
    pin35Enabled = enabled;
    pin35DefaultHigh = defaultHigh;
    pin35Pulsing = false; // Reset pulse state
    gpio_set_level(GPIO_PIN_35, enabled && defaultHigh ? 1 : 0);
    saveConfig();
}

void GPIOManager::setPin36Enabled(bool enabled, bool defaultHigh)
{
    pin36Enabled = enabled;
    pin36DefaultHigh = defaultHigh;
    pin36Pulsing = false; // Reset pulse state
    gpio_set_level(GPIO_PIN_36, enabled && defaultHigh ? 1 : 0);
    saveConfig();
}

bool GPIOManager::isPin35Enabled() const
{
    return pin35Enabled;
}

bool GPIOManager::isPin36Enabled() const
{
    return pin36Enabled;
}

bool GPIOManager::getPin35DefaultHigh() const
{
    return pin35DefaultHigh;
}

bool GPIOManager::getPin36DefaultHigh() const
{
    return pin36DefaultHigh;
}

void GPIOManager::setPin35PulseDuration(int duration)
{
    pin35PulseDuration = duration;
    saveConfig();
}

void GPIOManager::setPin36PulseDuration(int duration)
{
    pin36PulseDuration = duration;
    saveConfig();
}

int GPIOManager::getPin35PulseDuration() const
{
    return pin35PulseDuration;
}

int GPIOManager::getPin36PulseDuration() const
{
    return pin36PulseDuration;
}

void GPIOManager::pulsePin35()
{
    if (pin35Enabled)
    {
        // Reset pulse state if it was stuck
        if (pin35Pulsing)
        {
            gpio_set_level(GPIO_PIN_35, pin35DefaultHigh ? 1 : 0);
        }

        // Start new pulse
        gpio_set_level(GPIO_PIN_35, !pin35DefaultHigh ? 1 : 0); // Switch to opposite state
        pin35PulseStartTime = millis();
        pin35Pulsing = true;
    }
}

void GPIOManager::pulsePin36()
{
    if (pin36Enabled)
    {
        // Reset pulse state if it was stuck
        if (pin36Pulsing)
        {
            gpio_set_level(GPIO_PIN_36, pin36DefaultHigh ? 1 : 0);
        }

        // Start new pulse
        gpio_set_level(GPIO_PIN_36, !pin36DefaultHigh ? 1 : 0); // Switch to opposite state
        pin36PulseStartTime = millis();
        pin36Pulsing = true;
    }
}

void GPIOManager::loop()
{
    unsigned long currentTime = millis();

    // Handle Pin 35 pulse
    if (pin35Pulsing && (currentTime - pin35PulseStartTime >= pin35PulseDuration))
    {
        gpio_set_level(GPIO_PIN_35, pin35DefaultHigh ? 1 : 0); // Return to default state
        pin35Pulsing = false;
        pin35PulseStartTime = 0; // Reset timer
    }

    // Handle Pin 36 pulse
    if (pin36Pulsing && (currentTime - pin36PulseStartTime >= pin36PulseDuration))
    {
        gpio_set_level(GPIO_PIN_36, pin36DefaultHigh ? 1 : 0); // Return to default state
        pin36Pulsing = false;
        pin36PulseStartTime = 0; // Reset timer
    }

    // Safety check - reset if pulse has been active too long (5 seconds)
    if (pin35Pulsing && (currentTime - pin35PulseStartTime > 5000))
    {
        gpio_set_level(GPIO_PIN_35, pin35DefaultHigh ? 1 : 0);
        pin35Pulsing = false;
        pin35PulseStartTime = 0;
    }

    if (pin36Pulsing && (currentTime - pin36PulseStartTime > 5000))
    {
        gpio_set_level(GPIO_PIN_36, pin36DefaultHigh ? 1 : 0);
        pin36Pulsing = false;
        pin36PulseStartTime = 0;
    }
}

void GPIOManager::resetToDefaults()
{
    pin35Enabled = false;
    pin36Enabled = false;
    pin35DefaultHigh = false;
    pin36DefaultHigh = false;
    pin35PulseDuration = 1000;
    pin36PulseDuration = 1000;
    gpio_set_level(GPIO_PIN_35, 0);
    gpio_set_level(GPIO_PIN_36, 0);
    saveConfig();
}

bool GPIOManager::isInitialized() const
{
    return initialized;
}