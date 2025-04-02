#ifndef DEBUG_MANAGER_H
#define DEBUG_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

// Forward declaration of global debug state
extern bool DEBUG_ENABLED;

class DebugManager
{
public:
    DebugManager();
    void begin();
    void setDefaultDebug();
    void enableDebug();
    bool isDebugEnabled() const { return debugEnabled; }

    // Global debug state access
    static bool getGlobalDebugState() { return DEBUG_ENABLED; }
    static void setGlobalDebugState(bool state)
    {
        DEBUG_ENABLED = state;
        if (!state)
        {
            Serial.end();
        }
    }

    // Web interface methods
    bool updateDebugState(bool newState);
    bool getCurrentDebugState() const;

private:
    static const char *DEBUG_CONFIG_FILE;
    bool debugEnabled;
    bool readDebugConfig();
    bool writeDebugConfig(bool state);
};

extern DebugManager debugManager; // Global instance

#endif