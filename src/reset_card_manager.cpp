#include "reset_card_manager.h"
#include <WiFiManager.h>
#include "version_config.h"
#include "reader_manager.h"

ResetCardManager::ResetCardManager() {}

void ResetCardManager::begin()
{
    if (!LittleFS.exists(RESET_CARD_FILE))
    {
        setDefaultResetCard();
    }
}

void ResetCardManager::setDefaultResetCard()
{
    Serial.println("======================================================================");
    Serial.println("[RESET] Writing the default Reset Card values...");

    JsonDocument json;
    json["RBL"] = 35;
    json["RFC"] = 111;
    json["RCN"] = 4444;
    json["PAXTON_RESET_HEX"] = "0000001337";

    File resetCardFile = LittleFS.open(RESET_CARD_FILE, "w");
    if (!resetCardFile)
    {
        Serial.println("[RESET CARD] Failed to open Reset Card file for writing");
        return;
    }

    serializeJsonPretty(json, Serial);

    if (serializeJson(json, resetCardFile) == 0)
    {
        Serial.println(F("[RESET CARD] Failed to write the data to file"));
    }
    resetCardFile.close();
}

bool ResetCardManager::readResetCardConfig(int &resetBL, int &resetFC, int &resetCN)
{
    String dummyPaxtonHex;
    return readResetCardConfig(resetBL, resetFC, resetCN, dummyPaxtonHex);
}

bool ResetCardManager::readResetCardConfig(int &resetBL, int &resetFC, int &resetCN, String &paxtonHex)
{
    if (!LittleFS.exists(RESET_CARD_FILE))
    {
        Serial.println("[RESET] JSON file not found");
        return false;
    }

    File configFile = LittleFS.open(RESET_CARD_FILE, "r");
    if (!configFile)
    {
        Serial.println("[RESET] Failed to open config file");
        return false;
    }

    String configData = configFile.readString();
    configFile.close();

    JsonDocument jsonDoc;
    DeserializationError error = deserializeJson(jsonDoc, configData);
    if (error)
    {
        Serial.println("[RESET] Failed to parse config");
        return false;
    }

    resetBL = jsonDoc["RBL"];
    resetFC = jsonDoc["RFC"];
    resetCN = jsonDoc["RCN"];
    paxtonHex = jsonDoc["PAXTON_RESET_HEX"] | "0000001337";
    return true;
}

bool ResetCardManager::readPaxtonResetCard(String &paxtonHex)
{
    if (!LittleFS.exists(RESET_CARD_FILE))
    {
        Serial.println("[RESET] JSON file not found");
        return false;
    }

    File configFile = LittleFS.open(RESET_CARD_FILE, "r");
    if (!configFile)
    {
        Serial.println("[RESET] Failed to open config file");
        return false;
    }

    String configData = configFile.readString();
    configFile.close();

    JsonDocument jsonDoc;
    DeserializationError error = deserializeJson(jsonDoc, configData);
    if (error)
    {
        Serial.println("[RESET] Failed to parse config");
        return false;
    }

    paxtonHex = jsonDoc["PAXTON_RESET_HEX"] | "0000001337";
    return true;
}

void ResetCardManager::checkResetCard(CardProcessor &cardProcessor)
{
    if (readerManager.isPaxtonMode())
    {
        // Check for Paxton reset card
        String paxtonHex;
        if (!readPaxtonResetCard(paxtonHex))
        {
            return;
        }
        
        String cardHex = cardProcessor.getNet2HexEM410x();
        if (cardHex.equalsIgnoreCase(paxtonHex))
        {
            Serial.println("======================================================================");
            Serial.println("[RESET] Paxton Reset Card detected!");
            resetStoredWiFi();
        }
    }
    else
    {
        // Check for HID reset card
        int resetBL, resetFC, resetCN;
        String dummyPaxtonHex;
        if (!readResetCardConfig(resetBL, resetFC, resetCN, dummyPaxtonHex))
        {
            return;
        }

        if (cardProcessor.getBitCount() == resetBL &&
            cardProcessor.getFacilityCode() == resetFC &&
            cardProcessor.getCardNumber() == resetCN)
        {
            resetStoredWiFi();
        }
    }
}

void ResetCardManager::resetStoredWiFi()
{
    Serial.println("======================================================================");
    Serial.println("[RESET WIFI] Clearing stored WiFi Access Point...");
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    delay(3000);
    ESP.restart();
}

ResetCardManager resetCardManager;