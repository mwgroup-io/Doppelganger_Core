#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include "card_processor.h"
#include <mutex>

// File paths
#define FORMAT_LITTLEFS_IF_FAILED true
#define CARDS_CSV_FILE "/www/cards.csv"
#define LOGGER_RESET_CARD_FILE "/www/reset_card.json"
#define LOGGER_DEBUG_CONFIG_FILE "/www/debug_config.json"
#define NOTIFICATION_CONFIG_FILE "/notifications.json"

class Logger
{
public:
    static Logger &getInstance();
    void log(const char *message);
    const char *getCurrentTime();
    void consoleLog();
    void writeCardLog();
    void writePinLog();
    void writeKeypadLog();
    void logStartupBanner(const char *device, const char *version, const char *builddate, const char *hardware);
    void logWiFiInfo(const char *ssid, IPAddress ip, IPAddress gateway, const char *mac, int rssi);
    void logResetCardInfo(const char *resetCardFile);
    void logTimeInfo(time_t now);
    void logEmailStatus(bool enabled, const char *recipient);
    void logMDNSStatus(const char *host);
    void logLEDStatus(const char *message);
    void logBootComplete();
    void logFilesystemStatus(const char *message, bool success);
    void logGPIOStatus(const char *message);
    void logWebServerStatus(const char *message);
    void logWebServerURL(const char *prefix, const char *url);
    void logDebugStatus(const char *message);

private:
    void logCardDataNet2();
    void logCardDataStandard();
    void logCardDataPIV();
    void logCardDataPIN();
    void logCardDataKeypad();
    void logCardDataError();

    // Constructor and destructor
    Logger();
    ~Logger();

    // Prevent copying
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    // Implementation details
    char timeBuffer[20];
    std::mutex logMutex;
};

// Global instance
extern Logger &logger;

#endif