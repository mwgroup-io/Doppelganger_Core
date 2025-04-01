#include "debug_manager.h"
#include "version_config.h"
#include "logger.h"

const char *DebugManager::DEBUG_CONFIG_FILE = "/www/debug_config.json";

DebugManager::DebugManager() : debugEnabled(true)
{
    DEBUG_ENABLED = true; // Initialize global state
}

void DebugManager::begin()
{
    if (!LittleFS.exists(DEBUG_CONFIG_FILE))
    {
        logger.logDebugStatus("Debug config file not found. Setting defaults...");
        setDefaultDebug();
    }
    else
    {
        enableDebug();
    }
}

void DebugManager::setDefaultDebug()
{
    logger.logDebugStatus("Writing the default debug settings...");

    JsonDocument json;
    json["DEBUG"] = true;

    File debugFile = LittleFS.open(DEBUG_CONFIG_FILE, "w");
    if (!debugFile)
    {
        logger.logDebugStatus("Failed to open debug config file for writing");
        // Set debug to true by default if we can't write the file
        debugEnabled = true;
        DEBUG_ENABLED = true;
        return;
    }

    if (serializeJson(json, debugFile) == 0)
    {
        logger.logDebugStatus("Failed to write the data to file");
    }
    else
    {
        logger.logDebugStatus("Debug config file written successfully");
    }

    debugFile.close();

    // Set both local and global debug state
    debugEnabled = true;
    DEBUG_ENABLED = true;
}

bool DebugManager::readDebugConfig()
{
    File debugFile = LittleFS.open(DEBUG_CONFIG_FILE, "r");
    if (!debugFile)
    {
        logger.logDebugStatus("Failed to open debug config file");
        // Set debug to true by default if we can't read the file
        debugEnabled = true;
        DEBUG_ENABLED = true;
        return false;
    }

    // Create JSON document
    JsonDocument jsonDoc;
    DeserializationError error = deserializeJson(jsonDoc, debugFile);
    debugFile.close();

    if (error)
    {
        logger.logDebugStatus("Failed to parse debug config file");
        // Set debug to true by default if we can't parse the file
        debugEnabled = true;
        DEBUG_ENABLED = true;
        return false;
    }

    debugEnabled = jsonDoc["DEBUG"].as<bool>();
    DEBUG_ENABLED = debugEnabled; // Synchronize global state
    return true;
}

void DebugManager::enableDebug()
{
    if (!readDebugConfig())
    {
        return;
    }

    if (!debugEnabled)
    {
        logger.logDebugStatus("Debug mode disabled");
        Serial.end();
    }
    else
    {
        logger.logDebugStatus("Debug mode enabled");
    }
}

bool DebugManager::updateDebugState(bool newState)
{
    if (newState == debugEnabled)
        return true; // No change needed

    debugEnabled = newState;
    DEBUG_ENABLED = newState; // Update global state

    if (newState)
    {
        Serial.begin(115200);
        logger.logDebugStatus("Debug mode enabled");
    }
    else
    {
        logger.logDebugStatus("Debug mode disabled");
        Serial.end();
    }

    // Update config file with new state
    return writeDebugConfig(newState);
}

bool DebugManager::writeDebugConfig(bool state)
{
    if (!LittleFS.begin())
    {
        logger.logDebugStatus("Failed to mount LittleFS");
        return false;
    }

    JsonDocument json;
    json["DEBUG"] = state;

    File debugFile = LittleFS.open(DEBUG_CONFIG_FILE, "w");
    if (!debugFile)
    {
        logger.logDebugStatus("Failed to open debug config file for writing");
        return false;
    }

    if (serializeJson(json, debugFile) == 0)
    {
        logger.logDebugStatus("Failed to write debug config to file");
        debugFile.close();
        return false;
    }

    debugFile.close();
    return true;
}

DebugManager debugManager; // Global instance definition