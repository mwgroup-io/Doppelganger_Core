#ifndef WIFI_SETUP_MANAGER_H
#define WIFI_SETUP_MANAGER_H

#include <Arduino.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>
#include "logger.h"

// Default timestamp for time validation (1970-01-01 00:00:00)
#define ESP_TIME_DEFAULT_TS 0

class WiFiSetupManager
{
public:
    static WiFiSetupManager &getInstance();
    void begin(const char *device, const char *defaultPASS, const char *prefixSSID);
    bool isConnected() const;
    void resetStoredWiFi();
    void resetSettings(); // Add resetSettings method
    bool setupTime();     // Add time setup method
    const char *getSSID() const;
    IPAddress getLocalIP() const;
    IPAddress getGatewayIP() const;
    const char *getMacAddress() const;
    int getRSSI() const;

private:
    WiFiSetupManager();
    ~WiFiSetupManager();

    // Prevent copying
    WiFiSetupManager(const WiFiSetupManager &) = delete;
    WiFiSetupManager &operator=(const WiFiSetupManager &) = delete;

    WiFiManager wifiManager;
    bool connected;
    char ssid[50];
    IPAddress localIP;
    IPAddress gatewayIP;
    char macAddress[18];
    int rssi;
    void setupMDNS(const char *host);
    void setupWiFiManager(const char *deviceName, const char *defaultPass, const char *prefixSSID);
};

// Global instance
extern WiFiSetupManager &wifiSetupManager;

#endif // WIFI_SETUP_MANAGER_H