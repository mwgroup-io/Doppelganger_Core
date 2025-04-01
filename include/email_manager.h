#ifndef EMAIL_MANAGER_H
#define EMAIL_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <queue>
#include <mutex>
#include <base64.h>
#include <ArduinoJson.h>

// File paths
#define NOTIFICATION_CONFIG_FILE "/notifications.json"

class EmailManager
{
public:
    static EmailManager &getInstance();
    void begin(const char *host, const char *port, const char *user, const char *pass, const char *recipient);
    bool sendCardData(uint8_t bitCount, uint32_t facilityCode, uint32_t cardNumber, const String &ssid, const String &binData, const String &csvHEX, const String &reversedPairsUID);
    bool isConfigured() const { return is_configured; }
    EmailManager();
    ~EmailManager();
    void sendPinData(const String &pinCode, const String &ssid, const String &binData);
    void update();
    void readConfig();

protected:
    // Prevent copying
    EmailManager(const EmailManager &) = delete;
    EmailManager &operator=(const EmailManager &) = delete;

    // Email configuration
    static char smtp_host[35];
    static char smtp_user[35];
    static char smtp_pass[35];
    static char smtp_recipient[35];
    static char smtp_port[5]; // Changed from uint16_t to char[5]

    // PIN buffering
    String pinBuffer;
    unsigned long lastPinTime;
    static const unsigned long PIN_BUFFER_TIMEOUT = 3000; // 3 seconds timeout

    // Queue for email messages
    struct EmailMessage
    {
        enum class Type
        {
            CARD,
            PIN
        };

        Type type;
        uint8_t bitCount;
        unsigned long facilityCode;
        unsigned long cardNumber;
        String ssid;
        time_t timestamp;
        String binData;
        String csvHEX;
        String reversedPairsUID;
        String pinCode;
        String subject;
        String body;
    };

    std::queue<EmailMessage> messageQueue;
    std::mutex queueMutex;

    // Task handle for email processing
    TaskHandle_t emailTaskHandle;

    // Task function
    static void emailTaskFunction(void *parameter);

    // Helper functions
    bool processNextEmail();
    bool sendEmail(const char *to, const char *subject, const char *body);
    bool waitForResponse(WiFiClient &client, const char *expected, unsigned long timeout = 5000);
    void flushPinBuffer(const String &ssid);

    bool is_configured;
};

extern EmailManager emailManager; // Global instance declaration

#endif // EMAIL_MANAGER_H