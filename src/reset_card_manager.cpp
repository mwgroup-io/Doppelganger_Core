#include "reset_card_manager.h"
#include <WiFiManager.h>

const char *ResetCardManager::RESET_CARD_FILE = "/www/reset_card.json";

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

    File resetCardFile = LittleFS.open(RESET_CARD_FILE, "w");
    if (!resetCardFile)
    {
        Serial.println("[RESET CARD] Failed to open Reset Card file for writing");
        return;
    }

    // Print pretty version to Serial for debugging
    serializeJsonPretty(json, Serial);

    // Write compact version to file without extra newlines
    if (serializeJson(json, resetCardFile) == 0)
    {
        Serial.println(F("[RESET CARD] Failed to write the data to file"));
    }
    resetCardFile.close();
}

bool ResetCardManager::readResetCardConfig(int &resetBL, int &resetFC, int &resetCN)
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
    return true;
}

void ResetCardManager::checkResetCard(CardProcessor &cardProcessor)
{
    int resetBL, resetFC, resetCN;
    if (!readResetCardConfig(resetBL, resetFC, resetCN))
    {
        return;
    }

    // Toggle stealth mode on match
    if (cardProcessor.getBitCount() == resetBL &&
        cardProcessor.getFacilityCode() == resetFC &&
        cardProcessor.getCardNumber() == resetCN)
    {
        resetStoredWiFi();
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

ResetCardManager resetCardManager; // Global instance definition