#ifndef RESET_CARD_MANAGER_H
#define RESET_CARD_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "card_processor.h"

class ResetCardManager
{
public:
    ResetCardManager();
    void begin();
    void checkResetCard(CardProcessor &cardProcessor);
    void setDefaultResetCard();
    void resetStoredWiFi();
    bool readResetCardConfig(int &resetBL, int &resetFC, int &resetCN, String &paxtonHex);

private:
    bool readResetCardConfig(int &resetBL, int &resetFC, int &resetCN);
    bool readPaxtonResetCard(String &paxtonHex);
};

extern ResetCardManager resetCardManager; // Global instance declaration

#endif