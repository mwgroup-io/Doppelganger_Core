#pragma once

#include <Arduino.h>
#include "email_manager.h"
#include "logger.h"

class NotificationManager
{
public:
    static NotificationManager &getInstance();

    void begin();
    void handleCardRead(const char *cardData);
    void handlePinRead(const char *pinData);
    void handleWebhook(const char *data);
    void handleWebNotification(const char *data);
    void updateSettings(const char *data);

private:
    NotificationManager() = default;
    ~NotificationManager() = default;
    NotificationManager(const NotificationManager &) = delete;
    NotificationManager &operator=(const NotificationManager &) = delete;

    void processNotification(const char *data);
    void sendEmailNotification(const char *subject, const char *body);
    void sendWebhookNotification(const char *data);
    void sendWebNotification(const char *data);
};

// Define global instance
extern NotificationManager &notificationManager;