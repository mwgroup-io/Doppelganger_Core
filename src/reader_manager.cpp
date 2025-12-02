#include "reader_manager.h"
#include "wiegand_interface.h"
#include "net2_interface.h"

ReaderManager &readerManager = ReaderManager::getInstance();

ReaderManager::ReaderManager()
    : currentReaderType(READER_HID), interruptsInitialized(false)
{
}

ReaderManager::~ReaderManager() {}

ReaderManager &ReaderManager::getInstance()
{
    static ReaderManager instance;
    return instance;
}

void ReaderManager::begin()
{
    loadConfig();
    attachInterrupts();
}

void ReaderManager::loadConfig()
{
    Serial.println("======================================================================");
    Serial.println("[READER] Loading reader configuration...");

    if (!LittleFS.exists(READER_CONFIG_FILE))
    {
        Serial.println("[READER] Config file not found. Creating defaults...");
        setDefaultConfig();
        return;
    }

    File configFile = LittleFS.open(READER_CONFIG_FILE, "r");
    if (!configFile)
    {
        Serial.println("[READER] Failed to open config file");
        setDefaultConfig();
        return;
    }

    JsonDocument jsonDoc;
    DeserializationError error = deserializeJson(jsonDoc, configFile);
    configFile.close();

    if (error)
    {
        Serial.print("[READER] Failed to parse config file: ");
        Serial.println(error.c_str());
        setDefaultConfig();
        return;
    }

    String readerType = jsonDoc["READER_TYPE"].as<String>();

    if (readerType == "PAXTON")
    {
        currentReaderType = READER_PAXTON;
        Serial.println("[READER] Type: PAXTON");
    }
    else
    {
        currentReaderType = READER_HID;
        Serial.println("[READER] Type: HID");
    }
}

void ReaderManager::saveConfig(ReaderType type)
{
    JsonDocument json;
    json["READER_TYPE"] = (type == READER_PAXTON) ? "PAXTON" : "HID";

    File readerFile = LittleFS.open(READER_CONFIG_FILE, "w");
    if (readerFile)
    {
        serializeJson(json, readerFile);
        readerFile.close();
        Serial.println("[READER] Configuration saved");
    }
    else
    {
        Serial.println("[READER] Failed to save configuration");
    }
}

void ReaderManager::setDefaultConfig()
{
    Serial.println("[READER] Setting default configuration (HID)");

    JsonDocument json;
    json["READER_TYPE"] = "HID";

    File readerFile = LittleFS.open(READER_CONFIG_FILE, "w");
    if (readerFile)
    {
        serializeJson(json, readerFile);
        readerFile.close();
    }

    currentReaderType = READER_HID;
}

void ReaderManager::switchMode(ReaderType newType)
{
    Serial.println("======================================================================");
    Serial.print("[READER] Switching to ");
    Serial.println(newType == READER_PAXTON ? "PAXTON" : "HID");

    currentReaderType = newType;
    saveConfig(newType);
    attachInterrupts();
}

void ReaderManager::attachInterrupts()
{
    if (interruptsInitialized)
    {
        detachInterrupt(DATA0);
        detachInterrupt(DATA1);
        detachInterrupt(NET2_CLK_PIN);
    }

    if (currentReaderType == READER_PAXTON)
    {
        Serial.println("[READER] Attaching PAXTON NATIVE capture:");
        Serial.println("[READER]   - Net2 native: D1(CLK)/D0(DATA)");
        Serial.println("[READER]   - For Net2 tokens and KP75 keypad");

        pinMode(NET2_CLK_PIN, INPUT);
        pinMode(NET2_DATA_PIN, INPUT);
        attachInterrupt(NET2_CLK_PIN, ISR_NET2_CLK_FALLING, FALLING);
    }
    else
    {
        Serial.println("[READER] Attaching HID interrupt handlers");

        pinMode(DATA0, INPUT);
        pinMode(DATA1, INPUT);
        attachInterrupt(DATA0, handleData0, FALLING);
        attachInterrupt(DATA1, handleData1, FALLING);
    }

    interruptsInitialized = true;
}
