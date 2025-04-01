/*
 * Doppelgänger Pro v2 MWGroup Edition
 * Version 1.0.0 (22MAR2025)
 *
 * Professional RFID card cloning and analysis tool.
 * For authorized penetration testing use only.
 * Written by Travis Weathers (@tweathers-sec)
 */

#include <FS.h>
#include <LittleFS.h>
#include <Arduino.h>
#include "card_processor.h"
#include "logger.h"
#include "email_manager.h"
#include "reset_card_manager.h"
#include "debug_manager.h"
#include "wifi_setup_manager.h"
#include "version_config.h"
#include "email_config.h"
#include "wiegand_interface.h"
#include "gpio_manager.h"
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <ESPmDNS.h>
#include "wifi_manager_style.h"
#include "led_manager.h"
#include "notification_manager.h"
#include "websocket_handler.h"
#include "reset_manager.h"
#include "card_event_handler.h"

unsigned long startTime = 0;

// Global instances
extern CardProcessor cardProcessor;
extern EmailManager emailManager;
extern ResetCardManager resetCardManager;
extern DebugManager debugManager;
extern WiFiSetupManager &wifiSetupManager;
extern WiegandInterface &wiegandInterface;
extern NotificationManager &notificationManager;
extern CardEventHandler &cardEventHandler;
extern GPIOManager &gpioManager;

bool shouldSaveConfig = false;

WiFiManager wifiManager;

// mDNS Configuration
#define mdnsHost "RFID"

// Web servers
AsyncWebServer server(80);
WebSocketsServer websockets(81);

///////////////////////////////////////////////////////
// Functions start here
///////////////////////////////////////////////////////
/* Core Functions */
// System initialization
void setup()
{
  // Disable all ESP32 debug output
  esp_log_level_set("vfs_api", ESP_LOG_NONE);

  Serial.begin(115200);
  delay(2000);
  setCpuFrequencyMhz(CPU_FREQ_NORMAL);

  // Display startup banner
  logger.logStartupBanner(device, version, builddate, hardware);

  // Initialize filesystem
  logger.logFilesystemStatus("Initializing the filesystem...", true);
  if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED))
  {
    logger.logFilesystemStatus("LittleFS Mount Failed", false);
    return;
  }

  // Initialize debug settings and check state
  debugManager.begin();

  // Initialize LED manager
  LEDManager::getInstance().begin();

  // Initialize GPIO manager
  GPIOManager::getInstance().begin();

  // Configure hardware reset pins
  pinMode(RST, INPUT_PULLUP);
  ResetManager::getInstance().begin();

  // Initialize Wiegand interface
  logger.logGPIOStatus("Preparing GPIO configuration...");
  wiegandInterface.begin();
  cardProcessor.reset();
  logger.logGPIOStatus("GPIO configuration complete and ready");

  // Initialize WiFi
  Serial.println("======================================================================");
  wifiSetupManager.begin(device, defaultPASS, prefixSSID);

  if (!wifiSetupManager.isConnected())
  {
    Serial.println("[WIFI] Operating without WiFi connection");
  }
  else
  {
    // Wait for NTP sync
    if (wifiSetupManager.setupTime())
    {
      Serial.println("======================================================================");
      Serial.println("[WIFI] Successfully connected to WiFi");

      emailManager.readConfig();

      // Initialize email manager if configured
      if (emailManager.isConfigured())
      {
        emailManager.begin(smtp_host, smtp_port, smtp_user, smtp_pass, smtp_recipient);
        Serial.println("======================================================================");
      }

      if (WiFi.status() == WL_CONNECTED)
      {
        if (!MDNS.begin(mdnsHost))
        {
          Serial.println("======================================================================");
          Serial.println("[MDNS] Error setting up MDNS responder!");
        }
        else
        {
          Serial.println("======================================================================");
          Serial.println("[MDNS] MDNS responder started");
          MDNS.addService("http", "tcp", 80);
        }
        notificationManager.begin();
      }
    }
  }

  // Configure and start web server
  logger.logWebServerStatus("Starting web services");

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "*");
  DefaultHeaders::Instance().addHeader("Content-Encoding", "identity"); // Tell clients not to expect compression
  DefaultHeaders::Instance().addHeader("Accept-Encoding", "identity");  // Tell clients we only support uncompressed

  // Serve static files
  server.serveStatic("/", LittleFS, "/www/")
      .setDefaultFile("index.html")
      .setCacheControl("no-cache")
      .setLastModified("Mon, 20 Jun 2016 14:00:00 GMT");

  // Add firmware version endpoint
  server.on("/firmware", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    JsonDocument doc;
    doc["version"] = version;
    doc["buildDate"] = builddate;
    doc["device"] = device;
    serializeJson(doc, *response);
    request->send(response); });

  // Add notifications endpoint
  server.on("/notifications", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    JsonDocument doc;
    
    // Read current notification settings from file
    File notificationConfigFile = LittleFS.open(NOTIFICATION_CONFIG_FILE, "r");
    if (notificationConfigFile) {
      DeserializationError error = deserializeJson(doc, notificationConfigFile);
      notificationConfigFile.close();
      
      if (!error) {
        serializeJson(doc, *response);
      } else {
        doc["enable_email"] = "false";
        doc["smtp_host"] = "";
        doc["smtp_port"] = "";
        doc["smtp_user"] = "";
        doc["smtp_recipient"] = "";
        serializeJson(doc, *response);
      }
    } else {
      doc["enable_email"] = "false";
      doc["smtp_host"] = "";
      doc["smtp_port"] = "";
      doc["smtp_user"] = "";
      doc["smtp_recipient"] = "";
      serializeJson(doc, *response);
    }
    request->send(response); });

  // Add GPIO configuration endpoint
  server.on("/gpio.json", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    File file = LittleFS.open("/www/gpio.json", "r");
    if (file) {
      response->write(file);
      file.close();
    } else {
      JsonDocument doc;
      doc["pin35"] = false;
      doc["pin36"] = false;
      serializeJson(doc, *response);
    }
    request->send(response); });

  server.on("/gpio.json", HTTP_POST, [](AsyncWebServerRequest *request)
            {
    if (request->hasParam("data", true)) {
      String data = request->getParam("data", true)->value();
      File file = LittleFS.open("/www/gpio.json", "w");
      if (file) {
        file.print(data);
        file.close();
        request->send(200, "application/json", "{\"status\":\"success\"}");
      } else {
        request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Failed to write file\"}");
      }
    } else {
      request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"No data provided\"}");
    } });

  server.onNotFound([](AsyncWebServerRequest *request)
                    {
    if (request->method() == HTTP_OPTIONS)
    {
          request->send(200);
    }
    else
    {
          Serial.println("[WEBSERVER] 404 sent to client");
      request->send(404, "Not Found");
      } });

  server.begin();
  logger.logWebServerStatus("Webserver is running");
  websockets.begin();
  websockets.onEvent(webSocketEvent);
  logger.logWebServerStatus("WebSocket service is running");
  logger.logWebServerURL("Doppelgänger", "RFID.local");
  logger.logWebServerURL("Doppelgänger", wifiSetupManager.getLocalIP().toString().c_str());

  // Display current reset card config
  logger.logResetCardInfo(LOGGER_RESET_CARD_FILE);

  // Initialize reset card manager
  resetCardManager.begin();

  // Initialize card event handler
  cardEventHandler.begin();

  // Show LED status message
  logger.logLEDStatus("[LED] Informing of successful boot");

  // Run LED startup sequence
  LEDManager::getInstance().runStartupSequence();

  // Show boot complete message last
  logger.logBootComplete();
}

///////////////////////////////////////
// Main loop for processing card data, logging, and send e-mails
///////////////////////////////////////
void loop()
{
  static unsigned long firstPressTime = 0;
  static unsigned long debounceDelay = 100; // Reduced from 500ms to 100ms for faster PIN entry
  static bool waitingForSecondPress = false;

  // Update LED manager
  LEDManager::getInstance().update();

  // Process queued emails
  emailManager.update();

  // Handle hardware reset
  ResetManager::getInstance().update();

  // Update GPIO manager
  GPIOManager::getInstance().loop();

  // Process card data
  cardProcessor.processCard();

  if (cardProcessor.isReadComplete())
  {
    logger.consoleLog();
    resetCardManager.checkResetCard(cardProcessor);

    if (cardProcessor.getBitCount() == 4)
    {
      cardEventHandler.handlePinEvent(cardProcessor);
    }
    else
    {
      cardEventHandler.handleCardEvent(cardProcessor);
    }

    cardProcessor.reset();
  }

  websockets.loop();
}