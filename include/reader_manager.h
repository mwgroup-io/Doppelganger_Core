#ifndef READER_MANAGER_H
#define READER_MANAGER_H

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "version_config.h"

// Manages switching between HID and Paxton/Net2 reader modes

// Reader type enum
enum ReaderType
{
    READER_HID,
    READER_PAXTON
};

class ReaderManager
{
public:
    static ReaderManager &getInstance();

    // Initialize reader manager
    void begin();

    // Load reader configuration from file
    void loadConfig();

    // Save reader configuration to file
    void saveConfig(ReaderType type);

    // Set default configuration
    void setDefaultConfig();

    // Switch between HID and Paxton modes
    void switchMode(ReaderType newType);

    // Attach appropriate interrupts based on current reader type
    void attachInterrupts();

    // Get current reader type
    ReaderType getCurrentType() const { return currentReaderType; }

    // Check if current type is Paxton
    bool isPaxtonMode() const { return currentReaderType == READER_PAXTON; }

    // Check if current type is HID
    bool isHIDMode() const { return currentReaderType == READER_HID; }

private:
    ReaderManager();
    ~ReaderManager();

    // Prevent copying
    ReaderManager(const ReaderManager &) = delete;
    ReaderManager &operator=(const ReaderManager &) = delete;

    ReaderType currentReaderType;
    bool interruptsInitialized;
};

extern ReaderManager &readerManager;

#endif
