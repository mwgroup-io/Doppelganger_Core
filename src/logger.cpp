#include "logger.h"
#include <LittleFS.h>
#include <time.h>
#include <ArduinoJson.h>

enum class MessageType
{
    NONE,
    LEGAL,
    FILESYSTEM,
    DEBUG,
    GPIO,
    WEBSERVER,
    STARTUP
};

static MessageType lastMessageType = MessageType::NONE;

Logger::Logger()
{
    // Initialize time buffer
    timeBuffer[0] = '\0';
}

Logger::~Logger()
{
    // Destructor
}

Logger &Logger::getInstance()
{
    static Logger instance;
    return instance;
}

// Define global instance
Logger &logger = Logger::getInstance();

void Logger::log(const char *message)
{
    std::lock_guard<std::mutex> lock(logMutex);
    Serial.println(message);
}

const char *Logger::getCurrentTime()
{
    std::lock_guard<std::mutex> lock(logMutex);

    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return timeBuffer;
}

void Logger::consoleLog()
{
    Serial.println("======================================================================");
    if (cardProcessor.getBitCount() == 4)
    {
        logCardDataPIN();
    }
    else if (cardProcessor.getBitCount() == 32)
    {
        logCardDataPIV();
    }
    else if (cardProcessor.getBitCount() == 26 || cardProcessor.getBitCount() == 27 ||
             cardProcessor.getBitCount() == 28 || cardProcessor.getBitCount() == 29 ||
             cardProcessor.getBitCount() == 30 || cardProcessor.getBitCount() == 31 ||
             cardProcessor.getBitCount() == 33 || cardProcessor.getBitCount() == 34 ||
             cardProcessor.getBitCount() == 35 || cardProcessor.getBitCount() == 36 ||
             cardProcessor.getBitCount() == 37 || cardProcessor.getBitCount() == 48 ||
             cardProcessor.getBitCount() == 100 || cardProcessor.getBitCount() == 200)
    {
        logCardDataStandard();
    }
    else
    {
        logCardDataError();
    }
}

void Logger::logCardDataStandard()
{
    Serial.print("[CARD READ] Card Bits: ");
    Serial.print(cardProcessor.getBitCount());
    Serial.print(", FC = ");
    Serial.print(cardProcessor.getFacilityCode());
    Serial.print(", CN = ");
    Serial.print(cardProcessor.getCardNumber());
    Serial.print(", HEX = ");

    if ((cardProcessor.getBitCount() == 28 || 30 || 31 || 33 || 36 || 48) &&
        cardProcessor.getBitCount() != 26 && cardProcessor.getBitCount() != 27 &&
        cardProcessor.getBitCount() != 29 && cardProcessor.getBitCount() != 34 &&
        cardProcessor.getBitCount() != 35 && cardProcessor.getBitCount() != 37)
    {
        Serial.print(cardProcessor.getCsvHEX());
    }
    else if (cardProcessor.getBitCount() == 26 || 27 || 29 || 34 || 35 || 37)
    {
        Serial.print(cardProcessor.getCsvHEX());
    }

    Serial.print(", BIN = ");
    Serial.println(cardProcessor.getDataStreamBIN());
}

void Logger::logCardDataPIV()
{
    Serial.print("[CARD READ] Card Bits: ");
    if (cardProcessor.getFacilityCode() >= 256)
    {
        Serial.print("PIV/MF");
        Serial.print(", FC = UID");
        Serial.print(", CN = ");
        Serial.print(cardProcessor.getReversedPairsUID());
        Serial.print(", HEX = ");
        Serial.print(cardProcessor.getCsvHEX());
        Serial.print(", BIN = ");
        Serial.println(cardProcessor.getDataStreamBIN());
    }
    else
    {
        Serial.print(cardProcessor.getBitCount());
        Serial.print(", FC = ");
        Serial.print(cardProcessor.getFacilityCode());
        Serial.print(", CN = ");
        Serial.print(cardProcessor.getCardNumber());
        Serial.print(", HEX = ");
        Serial.print(cardProcessor.getCsvHEX());
        Serial.print(", BIN = ");
        Serial.println(cardProcessor.getDataStreamBIN());
    }
}

void Logger::logCardDataPIN()
{
    if (cardProcessor.getBitHolder1() == 10)
    {
        Serial.print("[PIN READ] PIN Code = *");
    }
    else if (cardProcessor.getBitHolder1() == 11)
    {
        Serial.print("[PIN READ] PIN Code = #");
    }
    else if (cardProcessor.getBitHolder1() >= 0 && cardProcessor.getBitHolder1() <= 9)
    {
        Serial.print("[PIN READ] PIN Code = ");
        Serial.print(cardProcessor.getCardChunk1(), HEX);
    }
    Serial.print(", BIN = ");
    Serial.println(cardProcessor.getDataStreamBIN());
}

void Logger::logCardDataError()
{
    std::lock_guard<std::mutex> lock(logMutex);
    Serial.println("[CARD READ] ERROR: Bad Card Read! Card data won't be displayed in the web log, but the data will be stored within the CSV file.");
    Serial.println("[CARD READ] POSSIBLE ISSUES:");
    Serial.println("[CARD READ]    (1) Card passed through the reader too quickly");
    Serial.println("[CARD READ]    (2) Loose GPIO connection(s)");
    Serial.println("[CARD READ]    (3) Electromagnetic interference (EMI)");
    Serial.println("[CARD READ]    (4) No available parser for card.");
    Serial.println("[CARD READ] Below is the bad data:");
    Serial.print("[CARD READ] Card Bits: ");
    Serial.print(cardProcessor.getBitCount());
    Serial.print(", FC = ");
    Serial.print(cardProcessor.getFacilityCode());
    Serial.print(", CN = ");
    Serial.print(cardProcessor.getCardNumber());
    Serial.print(", HEX = ");
    Serial.print(cardProcessor.getCsvHEX());
    Serial.print(", BIN = ");
    Serial.println(cardProcessor.getDataStreamBIN());

    File csvCards = LittleFS.open(CARDS_CSV_FILE, "a");
    if (!csvCards)
    {
        Serial.print("[LOG] There was an error opening ");
        Serial.println(CARDS_CSV_FILE);
        return;
    }

    csvCards.print("DATA_TYPE: NO_PARSER");
    csvCards.print(", Bit_Length: ");
    csvCards.print(cardProcessor.getBitCount());
    csvCards.print(", Hex_Value: ");
    csvCards.print(cardProcessor.getCsvHEX());
    csvCards.print(", Facility_Code: ");
    csvCards.print(cardProcessor.getFacilityCode(), DEC);
    csvCards.print(", Card_Number: ");
    csvCards.print(cardProcessor.getCardNumber(), DEC);
    csvCards.print(", BIN: ");
    csvCards.print(cardProcessor.getDataStreamBIN());
    csvCards.print("\n");
    csvCards.close();
}

void Logger::writeCardLog()
{
    Serial.print("[LOG] Logging card data to ");
    Serial.println(CARDS_CSV_FILE);
    File csvCards = LittleFS.open(CARDS_CSV_FILE, "a");

    if (!csvCards)
    {
        Serial.print("[LOG] There was an error opening ");
        Serial.println(CARDS_CSV_FILE);
        return;
    }

    if ((cardProcessor.getBitCount() == 26 || 27 || 28 || 29 || 30 || 31 || 33 || 34 || 35 || 36 || 37 || 48 || 100 || 200) &&
        cardProcessor.getBitCount() != 32)
    {
        csvCards.print("DATA_TYPE: CARD");
        csvCards.print(", Bit_Length: ");
        csvCards.print(cardProcessor.getBitCount());
        csvCards.print(", Hex_Value: ");

        if ((cardProcessor.getBitCount() == 28 || 30 || 31 || 33 || 36 || 48) &&
            cardProcessor.getBitCount() != 26 && cardProcessor.getBitCount() != 27 &&
            cardProcessor.getBitCount() != 29 && cardProcessor.getBitCount() != 34 &&
            cardProcessor.getBitCount() != 35 && cardProcessor.getBitCount() != 37)
        {
            csvCards.print(cardProcessor.getCsvHEX());
        }
        else if (cardProcessor.getBitCount() == 26 || 27 || 29 || 34 || 35 || 37)
        {
            csvCards.print(cardProcessor.getCsvHEX());
        }

        csvCards.print(", Facility_Code: ");
        csvCards.print(cardProcessor.getFacilityCode(), DEC);
        csvCards.print(", Card_Number: ");
        csvCards.print(cardProcessor.getCardNumber(), DEC);
        csvCards.print(", BIN: ");
        csvCards.print(cardProcessor.getDataStreamBIN());
    }
    else if (cardProcessor.getBitCount() == 32)
    {
        csvCards.print("DATA_TYPE: CARD");
        csvCards.print(", Bit_Length: ");
        if (cardProcessor.getFacilityCode() >= 256)
        {
            csvCards.print("PIV/MF");
            csvCards.print(", Hex_Value: ");
            csvCards.print(cardProcessor.getCsvHEX());
            csvCards.print(", Facility_Code: N/A");
            csvCards.print(", Card_Number: ");
            csvCards.print(cardProcessor.getReversedPairsUID());
        }
        else
        {
            csvCards.print(cardProcessor.getBitCount());
            csvCards.print(", Hex_Value: ");
            csvCards.print(cardProcessor.getCsvHEX());
            csvCards.print(", Facility_Code: ");
            csvCards.print(cardProcessor.getFacilityCode(), DEC);
            csvCards.print(", Card_Number: ");
            csvCards.print(cardProcessor.getCardNumber(), DEC);
        }
        csvCards.print(", BIN: ");
        csvCards.print(cardProcessor.getDataStreamBIN());
    }
    csvCards.print("\n");
    csvCards.close();
}

void Logger::writePinLog()
{
    String pinCode;
    if ((cardProcessor.getBitHolder1() == 10) && cardProcessor.getBitCount() == 4)
    {
        pinCode = "*";
    }
    else if ((cardProcessor.getBitHolder1() == 11) && cardProcessor.getBitCount() == 4)
    {
        pinCode = "#";
    }
    else if ((cardProcessor.getBitHolder1() >= 0 && cardProcessor.getBitHolder1() <= 9) &&
             cardProcessor.getBitCount() == 4)
    {
        pinCode = String(cardProcessor.getCardChunk1(), HEX);
    }
    else
    {
        Serial.println("[LOG] ERROR: PIN code not read");
    }

    Serial.print("[LOG] Logging PIN code to ");
    Serial.println(CARDS_CSV_FILE);
    File csvCards = LittleFS.open(CARDS_CSV_FILE, "a");

    if (!csvCards)
    {
        Serial.print("[LOG] There was an error opening ");
        Serial.println(CARDS_CSV_FILE);
        return;
    }

    csvCards.print("DATA_TYPE: KEYPAD");
    csvCards.print(", Bit_Length: PIN");
    csvCards.print(", Hex_Value: N/A");
    csvCards.print(", Facility_Code: N/A");
    csvCards.print(", Card_Number: ");
    csvCards.print(pinCode);
    csvCards.print(", BIN: ");
    csvCards.print(cardProcessor.getDataStreamBIN());
    csvCards.print("\n");
    csvCards.close();
}

void Logger::logStartupBanner(const char *device, const char *version, const char *builddate, const char *hardware)
{
    Serial.println("======================================================================");
    Serial.println(device);
    Serial.println("Copyright (c) 2025: Mayweather Group, LLC");
    Serial.print("Firmware Version: ");
    Serial.println(version);
    Serial.print("Build Date: ");
    Serial.println(builddate);
    Serial.print("Hardware REV: ");
    Serial.println(hardware);
    Serial.println("Firmware & Hardware: @tweathers-sec (GitHub) @tweathers_sec (X.com)");
    Serial.println("======================================================================");
    Serial.println("LEGAL DISCLAIMER:");
    Serial.println("This device is intended for professional penetration testing only.");
    Serial.println("Unauthorized or illegal use/possession of this device is the sole");
    Serial.println("responsibility of the user. Mayweather Group LLC, Practical Physical");
    Serial.println("Exploitation, and the creator are not liable for illegal application");
    Serial.println("of this device.");
    lastMessageType = MessageType::STARTUP;
}

void Logger::logWiFiInfo(const char *ssid, IPAddress ip, IPAddress gateway, const char *mac, int rssi)
{
    std::lock_guard<std::mutex> lock(logMutex);
    Serial.print("[WIFI] Successfully connected to WiFi");
    Serial.println("======================================================================");
}

void Logger::logResetCardInfo(const char *filePath)
{
    Serial.println("======================================================================");
    Serial.println("[RESET CARD] Current Reset Card information:");
    File file = LittleFS.open(filePath, "r");
    if (file)
    {
        // Read the file content
        String content = "";
        while (file.available())
        {
            content += (char)file.read();
        }
        file.close();

        // Parse the JSON to ensure proper formatting
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, content);
        if (!error)
        {
            // Print the JSON in a compact format
            serializeJson(doc, Serial);
            Serial.println(); // Add a single newline after the JSON
        }
        else
        {
            // If parsing fails, print the raw content
            Serial.println(content);
        }
    }
    Serial.println("======================================================================");
}

void Logger::logEmailStatus(bool enabled, const char *recipient)
{
    Serial.println("======================================================================");
    if (enabled)
    {
        Serial.print("[EMAIL] Notifications will be sent to: ");
        Serial.println(recipient);
    }
    else
    {
        Serial.println("[EMAIL] Notifications are currently disabled.");
    }
}

void Logger::logMDNSStatus(const char *host)
{
    Serial.println("======================================================================");
    Serial.println("[MDNS] MDNS responder started");
    Serial.print("[MDNS] Device will be reachable at http://");
    Serial.print(host);
    Serial.println(".local/");
}

void Logger::logLEDStatus(const char *message)
{
    std::lock_guard<std::mutex> lock(logMutex);
    Serial.println(message);
}

void Logger::logBootComplete()
{
    std::lock_guard<std::mutex> lock(logMutex);
    Serial.println("======================================================================");
    Serial.println(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
    Serial.println(">>>>>>>>>>>>>>>>>>>>>>>> BOOT PROCESS COMPLETE <<<<<<<<<<<<<<<<<<<<<<<");
    Serial.println(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
}

void Logger::logTimeInfo(time_t now)
{
    std::lock_guard<std::mutex> lock(logMutex);
    char timeStr[26];
    ctime_r(&now, timeStr);
    timeStr[24] = '\0'; // Remove the newline
    Serial.print("[NTP] The current time is: ");
    Serial.print(timeStr);
    Serial.println(" UTC");
}

void Logger::logFilesystemStatus(const char *message, bool success)
{
    std::lock_guard<std::mutex> lock(logMutex);
    if (lastMessageType != MessageType::FILESYSTEM)
    {
        Serial.println("======================================================================");
        lastMessageType = MessageType::FILESYSTEM;
    }
    Serial.print("[FILESYSTEM] ");
    Serial.println(message);
}

void Logger::logGPIOStatus(const char *message)
{
    std::lock_guard<std::mutex> lock(logMutex);
    if (lastMessageType != MessageType::GPIO)
    {
        Serial.println("======================================================================");
        lastMessageType = MessageType::GPIO;
    }
    Serial.print("[GPIO] ");
    Serial.println(message);
}

void Logger::logWebServerStatus(const char *message)
{
    std::lock_guard<std::mutex> lock(logMutex);
    if (lastMessageType != MessageType::WEBSERVER)
    {
        Serial.println("======================================================================");
        lastMessageType = MessageType::WEBSERVER;
    }
    Serial.print("[WEBSERVER] ");
    Serial.println(message);
}

void Logger::logWebServerURL(const char *prefix, const char *url)
{
    std::lock_guard<std::mutex> lock(logMutex);
    Serial.print("[WEBSERVER] ");
    Serial.print(prefix);
    Serial.print(": http://");
    Serial.print(url);
    Serial.println("/");
}

void Logger::logDebugStatus(const char *message)
{
    std::lock_guard<std::mutex> lock(logMutex);
    if (lastMessageType != MessageType::DEBUG)
    {
        Serial.println("======================================================================");
        lastMessageType = MessageType::DEBUG;
    }
    Serial.print("[DEBUG] ");
    Serial.println(message);
}