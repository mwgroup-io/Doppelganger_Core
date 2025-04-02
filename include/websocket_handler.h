#pragma once

#include <Arduino.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "reset_card_manager.h"
#include "email_manager.h"
#include "wifi_setup_manager.h"
#include "debug_manager.h"
#include "logger.h"
#include "version_config.h"
#include "notification_manager.h"

// Declare websockets as extern since it's defined in main.cpp
extern WebSocketsServer websockets;

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);