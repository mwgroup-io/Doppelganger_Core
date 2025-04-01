#include "notification_manager.h"
#include <ArduinoJson.h>
#include "email_config.h"
#include <WiFi.h>

NotificationManager &NotificationManager::getInstance()
{
    static NotificationManager instance;
    return instance;
}

void NotificationManager::begin()
{
    // No need to read email config here as it's already done by emailManager
    // Just initialize any other notification settings if needed
}

void NotificationManager::handleCardRead(const char *cardData)
{
    if (emailManager.isConfigured())
    {
        // Parse card data from CSV format
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

            // Send email notification with original format
            emailManager.sendCardData(bitCount, facilityCode, cardNumber, WiFi.SSID(), binData, csvHEX, "");
        }
    }
    processNotification(cardData);
}

void NotificationManager::handlePinRead(const char *pinData)
{
    if (emailManager.isConfigured())
    {
        // Parse pin data from CSV format
        String data = String(pinData);
        int comma = data.indexOf(',');
        if (comma != -1)
        {
            String pinCode = data.substring(0, comma);
            String binData = data.substring(comma + 1);

            // Send email notification with original format
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
    // Parse notification settings from JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data);
    if (!error)
    {
        // Update email settings
        if (doc["email"].is<JsonObject>())
        {
            emailManager.readConfig();
        }
        // Add other notification settings as needed
    }
}

void NotificationManager::processNotification(const char *data)
{
    // Process notification data and send to appropriate channels
    // This could include webhook, web notifications, etc.
}

void NotificationManager::sendEmailNotification(const char *subject, const char *body)
{
    if (emailManager.isConfigured())
    {
        // For general notifications, we'll use sendCardData with a special format
        emailManager.sendCardData(0, 0, 0, WiFi.SSID(), body, "", "");
    }
}

void NotificationManager::sendWebhookNotification(const char *data)
{
    // Implement webhook notification logic
}

void NotificationManager::sendWebNotification(const char *data)
{
    // Implement web notification logic
}

// Define global instance
NotificationManager &notificationManager = NotificationManager::getInstance();