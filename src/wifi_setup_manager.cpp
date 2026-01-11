#include "wifi_setup_manager.h"
#include "wifi_manager_style.h"
#include "version_config.h"
#include <time.h>

WiFiSetupManager::WiFiSetupManager() : connected(false), rssi(0)
{
  ssid[0] = '\0';
  macAddress[0] = '\0';
}

WiFiSetupManager::~WiFiSetupManager()
{
}

WiFiSetupManager &WiFiSetupManager::getInstance()
{
  static WiFiSetupManager instance;
  return instance;
}

// Global instance
WiFiSetupManager &wifiSetupManager = WiFiSetupManager::getInstance();

void WiFiSetupManager::begin(const char *deviceName, const char *defaultPass, const char *prefixSSID)
{
  setupWiFiManager(deviceName, defaultPass, prefixSSID);
  if (connected)
  {
    setupMDNS("rfid");
  }
}

void WiFiSetupManager::setupWiFiManager(const char *deviceName, const char *defaultPass, const char *prefixSSID)
{
  // Get MAC address for unique SSID
  uint8_t mac[6];
  WiFi.macAddress(mac);

  // Set the unique SSID using String concatenation
  char lastFourDigits[5];
  sprintf(lastFourDigits, "%02X%02X", mac[4], mac[5]);
  String defaultSSID = String(prefixSSID) + String(lastFourDigits);
  strncpy(ssid, defaultSSID.c_str(), sizeof(ssid) - 1);
  ssid[sizeof(ssid) - 1] = '\0'; // Ensure null termination

  // Set WiFi hostname BEFORE connecting (critical for Android mDNS compatibility)
  WiFi.setHostname("rfid");

  // Configure WiFiManager
  wifiManager.setHostname("rfid");
  wifiManager.setConfigPortalTimeout(180); // 3 minutes
  wifiManager.setMinimumSignalQuality(65);
  wifiManager.setDebugOutput(true);
  wifiManager.setConnectTimeout(30);
  wifiManager.setScanDispPerc(true);
  wifiManager.setSaveConfigCallback([]()
                                    { ESP.restart(); });

  // Set menu options
  std::vector<const char *> menu = {"wifi",  "sep", "info", "sep", "restart", "exit"};
  wifiManager.setMenu(menu);

  // Set title and theme
  wifiManager.setTitle(deviceName);
  wifiManager.setClass("invert"); // Enables dark theme base

  // Add custom CSS
  const char *customCSS = WIFI_MANAGER_CUSTOM_CSS;
  wifiManager.setCustomHeadElement(customCSS);

  // Attempt to connect
  connected = wifiManager.autoConnect(ssid, defaultPass);
  if (connected)
  {
    // Ensure hostname is set after connection (Android compatibility)
    WiFi.setHostname("rfid");
    localIP = WiFi.localIP();
    gatewayIP = WiFi.gatewayIP();
    strncpy(macAddress, WiFi.macAddress().c_str(), sizeof(macAddress) - 1);
    rssi = WiFi.RSSI();
  }
}

void WiFiSetupManager::setupMDNS(const char *host)
{
  // Ensure hostname matches mDNS name exactly (Android is case-sensitive)
  WiFi.setHostname(host);
  delay(100);
  
  if (!MDNS.begin(host))
  {
    Serial.println("[MDNS] Error setting up MDNS responder!");
  }
  else
  {
    MDNS.addService("http", "tcp", 80);
    Serial.println("[MDNS] MDNS responder started");
    Serial.print("[MDNS] Device will be reachable at http://");
    Serial.print(host);
    Serial.println(".local/");
  }
}

void WiFiSetupManager::resetSettings()
{
  Serial.println("======================================");
  Serial.println("[RESET WIFI] Clearing stored WiFi Access Point...");
  wifiManager.resetSettings();
  delay(3000);
  ESP.restart();
}

void WiFiSetupManager::resetStoredWiFi()
{
  Serial.println("======================================================================");
  resetSettings();
}

bool WiFiSetupManager::isConnected() const
{
  return connected;
}

const char *WiFiSetupManager::getSSID() const
{
  return ssid;
}

IPAddress WiFiSetupManager::getLocalIP() const
{
  return localIP;
}

IPAddress WiFiSetupManager::getGatewayIP() const
{
  return gatewayIP;
}

const char *WiFiSetupManager::getMacAddress() const
{
  return macAddress;
}

int WiFiSetupManager::getRSSI() const
{
  return rssi;
}

bool WiFiSetupManager::setupTime()
{
  Serial.println("======================================================================");
  Serial.println("[NTP] Waiting for NTP server to synchronize.");
  configTime(3, 0, "pool.ntp.org", "time.nist.gov");
  unsigned long ms = millis();

  time_t now = time(nullptr);
  while (now < ESP_TIME_DEFAULT_TS && millis() - ms < 10000)
  {
    delay(100);
    now = time(nullptr);
  }

  if (now > ESP_TIME_DEFAULT_TS)
  {
    char timeStr[26];
    ctime_r(&now, timeStr);
    timeStr[24] = '\0'; // Remove the newline
    Serial.print("[NTP] The current time is: ");
    Serial.print(timeStr);
    Serial.println(" UTC");
  }

  return (now > ESP_TIME_DEFAULT_TS);
}
