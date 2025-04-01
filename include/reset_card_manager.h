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

private:
    static const char *RESET_CARD_FILE;
    bool readResetCardConfig(int &resetBL, int &resetFC, int &resetCN);
};

extern ResetCardManager resetCardManager; // Global instance declaration

#endif // RESET_CARD_MANAGER_H