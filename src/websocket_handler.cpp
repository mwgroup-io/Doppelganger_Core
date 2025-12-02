#include "websocket_handler.h"
#include "reader_manager.h"
#include "gpio_manager.h"
#include "card_processor.h"

// Declare external notification manager instance
extern NotificationManager &notificationManager;
extern ReaderManager &readerManager;
extern CardProcessor cardProcessor;

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
    String message;

    switch (type)
    {
    case WStype_DISCONNECTED:
        break;
    case WStype_CONNECTED:
    {
        IPAddress ip = websockets.remoteIP(num);
        websockets.sendTXT(num, "Connected to Doppelgänger server.");
    }
    break;
    case WStype_TEXT:
        message = String((char *)(payload));

        if (DEBUG_ENABLED)
        {
            Serial.println("======================================================================");
            Serial.printf("[WEBSOCKET] Client sent instructions: ");
            Serial.println(message);
        }

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, message);

        if (error)
        {
            if (DEBUG_ENABLED)
            {
                logger.logDebugStatus("deserializeJson() failed");
            }
            return;
        }

        // Handle debug state changes
        if (doc["DEBUG"].is<bool>())
        {
            bool newDebugState = doc["DEBUG"].as<bool>();
            if (debugManager.updateDebugState(newDebugState))
            {
                if (DEBUG_ENABLED)
                {
                    logger.logDebugStatus("Debug settings updated successfully");
                }
                ESP.restart();
            }
            else
            {
                if (DEBUG_ENABLED)
                {
                    logger.logDebugStatus("Failed to update debug settings");
                }
            }
        }

        // Handle reader configuration changes
        if (doc["READER_TYPE"].is<const char *>())
        {
            String readerType = doc["READER_TYPE"].as<String>();
            ReaderType newType = (readerType == "PAXTON") ? READER_PAXTON : READER_HID;
            
            Serial.println("======================================================================");
            Serial.print("[WEBSOCKET] Changing reader type to: ");
            Serial.println(readerType);
            
            readerManager.switchMode(newType);
            
            JsonDocument response;
            response["status"] = "success";
            response["reader_type"] = readerType;
            String responseStr;
            serializeJson(response, responseStr);
            websockets.sendTXT(num, responseStr);
            
            Serial.println("[WEBSOCKET] Reader configuration updated successfully.");
        }

        // Handle GPIO configuration
        if (doc["pin35_enabled"].is<bool>() || doc["pin36_enabled"].is<bool>() ||
            doc["pin35_pulse_duration"].is<int>() || doc["pin36_pulse_duration"].is<int>())
        {
            bool pin35Enabled = doc["pin35_enabled"] | false;
            bool pin36Enabled = doc["pin36_enabled"] | false;
            bool pin35DefaultHigh = doc["pin35_default_high"] | false;
            bool pin36DefaultHigh = doc["pin36_default_high"] | false;
            int pin35PulseDuration = doc["pin35_pulse_duration"] | 1000;
            int pin36PulseDuration = doc["pin36_pulse_duration"] | 1000;

            GPIOManager::getInstance().setPin35Enabled(pin35Enabled, pin35DefaultHigh);
            GPIOManager::getInstance().setPin36Enabled(pin36Enabled, pin36DefaultHigh);
            GPIOManager::getInstance().setPin35PulseDuration(pin35PulseDuration);
            GPIOManager::getInstance().setPin36PulseDuration(pin36PulseDuration);

            cardProcessor.setPin35OnCardRead(pin35Enabled);
            cardProcessor.setPin36OnCardRead(pin36Enabled);

            File gpioConfigFile = LittleFS.open("/www/gpio.json", "w");
            if (gpioConfigFile)
            {
                JsonDocument gpioDoc;
                gpioDoc["pin35_enabled"] = pin35Enabled;
                gpioDoc["pin36_enabled"] = pin36Enabled;
                gpioDoc["pin35_default_high"] = pin35DefaultHigh;
                gpioDoc["pin36_default_high"] = pin36DefaultHigh;
                gpioDoc["pin35_pulse_duration"] = pin35PulseDuration;
                gpioDoc["pin36_pulse_duration"] = pin36PulseDuration;
                serializeJson(gpioDoc, gpioConfigFile);
                gpioConfigFile.close();
            }

            JsonDocument response;
            response["status"] = "success";
            String responseStr;
            serializeJson(response, responseStr);
            websockets.sendTXT(num, responseStr);
        }

        // Handle GPIO reset
        if (doc["reset_gpio"] == true)
        {
            Serial.println("======================================================================");
            Serial.println("[WEBSOCKET] Resetting GPIO settings to factory defaults...");
            GPIOManager::getInstance().resetToDefaults();
            cardProcessor.setPin35OnCardRead(false);
            cardProcessor.setPin36OnCardRead(false);
            Serial.println("======================================================================");
            Serial.println("[WEBSOCKET] GPIO settings have been restored to factory defaults.");

            JsonDocument response;
            response["source"] = "gpio";
            response["status"] = "success";
            String responseStr;
            serializeJson(response, responseStr);
            websockets.sendTXT(num, responseStr);
        }

        bool erase_cards = doc["WIPE_CARDS"];
        bool reset_wireless = doc["WIPE_WIFI"];
        bool restore_notifications_config = doc["WIPE_CONFIG"];
        String notification_settings = doc["enable_email"];

        bool default_reset_card = doc["RESET_CARD"];
        bool hasRBL = doc["RBL"].is<const char *>() || doc["RBL"].is<int>();
        bool hasRFC = doc["RFC"].is<const char *>() || doc["RFC"].is<int>();
        bool hasRCN = doc["RCN"].is<const char *>() || doc["RCN"].is<int>();
        bool hasPaxtonResetHex = doc["PAXTON_RESET_HEX"].is<const char *>();

        bool default_settings = doc["RESET_DEVICE"];

        if (erase_cards)
        {
            Serial.println("======================================================================");
            Serial.println("[WEBSOCKET] Clearing stored cards from the device...");
            LittleFS.remove(CARDS_CSV_FILE);
            delay(1000);
            File csvCards = LittleFS.open(CARDS_CSV_FILE, "w");
            csvCards.close();

            Serial.println("======================================================================");
            Serial.println("[WEBSOCKET] Stored card data has been cleared.");

            JsonDocument response;
            response["source"] = "cards";
            response["status"] = "success";
            String responseStr;
            serializeJson(response, responseStr);
            websockets.sendTXT(num, responseStr);
        }

        if (restore_notifications_config)
        {
            Serial.println("======================================================================");
            Serial.println("[WEBSOCKET] Restoring notification configuration to factory defaults...");

            LittleFS.remove(NOTIFICATION_CONFIG_FILE);
            delay(1000);

            JsonDocument notifyDoc;
            notifyDoc["enable_email"] = false;
            notifyDoc["smtp_host"] = "";
            notifyDoc["smtp_port"] = "";
            notifyDoc["smtp_user"] = "";
            notifyDoc["smtp_recipient"] = "";

            File jsonConfig = LittleFS.open(NOTIFICATION_CONFIG_FILE, "w");
            if (jsonConfig)
            {
                serializeJson(notifyDoc, jsonConfig);
                jsonConfig.close();
            }

            Serial.println("======================================================================");
            Serial.println("[WEBSOCKET] Notification preferences have been restored to defaults.");

            emailManager.readConfig();

            JsonDocument response;
            response["source"] = "notifications";
            response["status"] = "success";
            String responseStr;
            serializeJson(response, responseStr);
            websockets.sendTXT(num, responseStr);
        }

        if (default_reset_card)
        {
            Serial.println("======================================================================");
            Serial.println("[WEBSOCKET] Restoring the Reset Card to the default values...");
            LittleFS.remove(RESET_CARD_FILE);
            delay(1000);
            File resetCardFile = LittleFS.open(RESET_CARD_FILE, "w");
            resetCardFile.close();
            resetCardManager.setDefaultResetCard();
            Serial.println("");
            Serial.println("======================================================================");
            Serial.println("[WEBSOCKET] Reset Card default values have been restored.");

            JsonDocument response;
            response["source"] = "reset_card";
            response["status"] = "success";
            String responseStr;
            serializeJson(response, responseStr);
            websockets.sendTXT(num, responseStr);
        }

        if (reset_wireless)
        {
            Serial.println("======================================================================");
            Serial.println("[WEBSOCKET] Removing stored wireless credentials...");
            wifiSetupManager.resetStoredWiFi();

            JsonDocument response;
            response["source"] = "wireless";
            response["status"] = "success";
            String responseStr;
            serializeJson(response, responseStr);
            websockets.sendTXT(num, responseStr);
        }

        if (default_settings)
        {
            Serial.println("======================================================================");
            Serial.println("[WEBSOCKET] Restoring factory defaults...");

            LittleFS.remove(CARDS_CSV_FILE);
            delay(1000);
            File csvCards = LittleFS.open(CARDS_CSV_FILE, "w");
            csvCards.close();
            Serial.println("[WEBSOCKET] Stored card data has been cleared.");

            Serial.println("======================================================================");
            Serial.println("[WEBSOCKET] Resetting notification settings to factory defaults...");
            LittleFS.remove(NOTIFICATION_CONFIG_FILE);
            delay(1000);
            JsonDocument notifyDoc;
            notifyDoc["enable_email"] = false;
            notifyDoc["smtp_host"] = "";
            notifyDoc["smtp_port"] = "";
            notifyDoc["smtp_user"] = "";
            notifyDoc["smtp_recipient"] = "";
            File jsonConfig = LittleFS.open(NOTIFICATION_CONFIG_FILE, "w");
            if (jsonConfig)
            {
                serializeJson(notifyDoc, jsonConfig);
                jsonConfig.close();
            }
            emailManager.readConfig();
            Serial.println("[WEBSOCKET] Notification settings have been restored.");

            Serial.println("======================================================================");
            Serial.println("[WEBSOCKET] Resetting GPIO settings to factory defaults...");
            GPIOManager::getInstance().resetToDefaults();
            cardProcessor.setPin35OnCardRead(false);
            cardProcessor.setPin36OnCardRead(false);
            Serial.println("[WEBSOCKET] GPIO settings have been restored to factory defaults.");

            Serial.println("======================================================================");
            Serial.println("[WEBSOCKET] Restoring the Reset Card to default values...");
            LittleFS.remove(RESET_CARD_FILE);
            delay(1000);
            resetCardManager.setDefaultResetCard();
            Serial.println("");
            Serial.println("[WEBSOCKET] Reset Card default values have been restored.");

            Serial.println("======================================================================");
            Serial.println("[WEBSOCKET] Removing stored WiFi credentials...");
            wifiSetupManager.resetSettings();

            JsonDocument response;
            response["source"] = "system";
            response["status"] = "success";
            String responseStr;
            serializeJson(response, responseStr);
            websockets.sendTXT(num, responseStr);

            Serial.println("======================================================================");
            Serial.println("[WEBSOCKET] Reset device to factory defaults. Restarting the device.");
            delay(3000);
            ESP.restart();
        }

        if (notification_settings == "true" || notification_settings == "false")
        {
            Serial.println("======================================================================");
            Serial.println("[WEBSOCKET] Saving notification configuration...");

            JsonDocument notificationDoc;
            notificationDoc["enable_email"] = doc["enable_email"];
            notificationDoc["smtp_host"] = doc["smtp_host"];
            notificationDoc["smtp_port"] = doc["smtp_port"];
            notificationDoc["smtp_user"] = doc["smtp_user"];
            notificationDoc["smtp_pass"] = doc["smtp_pass"];
            notificationDoc["smtp_recipient"] = doc["smtp_recipient"];

            File notificationConfigFile = LittleFS.open(NOTIFICATION_CONFIG_FILE, "w");
            if (!notificationConfigFile)
            {
                Serial.println("[WEBSOCKET] Failed to open configuration file for writing");
            }

            serializeJsonPretty(notificationDoc, Serial);
            if (serializeJson(notificationDoc, notificationConfigFile) == 0)
            {
                Serial.println(F("[WEBSOCKET] Failed to write data to config file"));
            }
            notificationConfigFile.close();
            Serial.println("");
            Serial.println("======================================================================");
            Serial.println("[WEBSOCKET] File successfully written");

            JsonDocument response;
            response["status"] = "success";
            String responseStr;
            serializeJson(response, responseStr);
            websockets.sendTXT(num, responseStr);
        }

        if (hasRBL && hasRFC && hasRCN)
        {
            int resetBL = doc["RBL"].as<int>();
            int resetFC = doc["RFC"].as<int>();
            int resetCN = doc["RCN"].as<int>();

            Serial.println("======================================================================");
            Serial.println("[WEBSOCKET] Updating Reset Card file...");
            File resetCardFile = LittleFS.open(RESET_CARD_FILE, "r");
            JsonDocument resetDoc;
            String existingPaxtonHex = "0000001337";
            
            // Read existing file to preserve Paxton reset hex if it exists
            if (resetCardFile)
            {
                String existingData = resetCardFile.readString();
                resetCardFile.close();
                JsonDocument existingDoc;
                DeserializationError error = deserializeJson(existingDoc, existingData);
                if (!error)
                {
                    existingPaxtonHex = existingDoc["PAXTON_RESET_HEX"] | "0000001337";
                }
            }
            
            resetCardFile = LittleFS.open(RESET_CARD_FILE, "w");
            if (resetCardFile)
            {
                resetDoc["RBL"] = resetBL;
                resetDoc["RFC"] = resetFC;
                resetDoc["RCN"] = resetCN;
                resetDoc["PAXTON_RESET_HEX"] = existingPaxtonHex;
                serializeJson(resetDoc, resetCardFile);
                resetCardFile.close();
                Serial.println("[RESET] Writing the default Reset Card values...");
                serializeJson(resetDoc, Serial);
                Serial.println();
                Serial.println("======================================================================");
                Serial.println("[WEBSOCKET] Successfully updated Reset Card file");

                JsonDocument response;
                response["status"] = "success";
                String responseStr;
                serializeJson(response, responseStr);
                websockets.sendTXT(num, responseStr);
            }
            else
            {
                Serial.println("[WEBSOCKET] Failed to open the JSON file for writing.");
                JsonDocument response;
                response["status"] = "error";
                response["message"] = "Failed to open Reset Card file for writing";
                String responseStr;
                serializeJson(response, responseStr);
                websockets.sendTXT(num, responseStr);
            }
        }

        if (hasPaxtonResetHex)
        {
            String hex = doc["PAXTON_RESET_HEX"].as<String>();
            hex.toUpperCase();
            
            // Validate hex format (must be 10 characters, hex only)
            if (hex.length() != 10)
            {
                JsonDocument response;
                response["status"] = "error";
                response["message"] = "HEX value must be exactly 10 characters";
                String responseStr;
                serializeJson(response, responseStr);
                websockets.sendTXT(num, responseStr);
                return;
            }
            
            bool validHex = true;
            for (int i = 0; i < hex.length(); i++)
            {
                char c = hex.charAt(i);
                if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F')))
                {
                    validHex = false;
                    break;
                }
            }
            
            if (!validHex)
            {
                JsonDocument response;
                response["status"] = "error";
                response["message"] = "HEX value must contain only hexadecimal characters (0-9, A-F)";
                String responseStr;
                serializeJson(response, responseStr);
                websockets.sendTXT(num, responseStr);
                return;
            }

            Serial.println("======================================================================");
            Serial.println("[WEBSOCKET] Updating Paxton Reset Card...");
            File resetCardFile = LittleFS.open(RESET_CARD_FILE, "r");
            JsonDocument resetDoc;
            int existingRBL = 35, existingRFC = 111, existingRCN = 4444;
            
            // Read existing file to preserve HID reset card values
            if (resetCardFile)
            {
                String existingData = resetCardFile.readString();
                resetCardFile.close();
                JsonDocument existingDoc;
                DeserializationError error = deserializeJson(existingDoc, existingData);
                if (!error)
                {
                    existingRBL = existingDoc["RBL"] | 35;
                    existingRFC = existingDoc["RFC"] | 111;
                    existingRCN = existingDoc["RCN"] | 4444;
                }
            }
            
            resetCardFile = LittleFS.open(RESET_CARD_FILE, "w");
            if (resetCardFile)
            {
                resetDoc["RBL"] = existingRBL;
                resetDoc["RFC"] = existingRFC;
                resetDoc["RCN"] = existingRCN;
                resetDoc["PAXTON_RESET_HEX"] = hex;
                serializeJson(resetDoc, resetCardFile);
                resetCardFile.close();
                Serial.print("[RESET] Paxton Reset Card HEX updated to: ");
                Serial.println(hex);
                Serial.println("======================================================================");
                Serial.println("[WEBSOCKET] Successfully updated Paxton Reset Card");

                JsonDocument response;
                response["status"] = "success";
                String responseStr;
                serializeJson(response, responseStr);
                websockets.sendTXT(num, responseStr);
            }
            else
            {
                Serial.println("[WEBSOCKET] Failed to open the JSON file for writing.");
                JsonDocument response;
                response["status"] = "error";
                response["message"] = "Failed to open Reset Card file for writing";
                String responseStr;
                serializeJson(response, responseStr);
                websockets.sendTXT(num, responseStr);
            }
        }

        doc.clear();

        delay(1000);
        if (reset_wireless || default_settings)
        {
            ESP.restart();
        }
    }
}