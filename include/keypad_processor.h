#ifndef KEYPAD_PROCESSOR_H
#define KEYPAD_PROCESSOR_H

#include <Arduino.h>
#include "version_config.h"

// Paxton KP75 Keypad Support
// Format: cXXe where XX is key number (01-12, etc)
// Data stream: 55-56 bits via Net2 protocol

#define KEYPAD_MIN_BITS 55
#define KEYPAD_MAX_BITS 56

class KeypadProcessor
{
public:
    static KeypadProcessor &getInstance();

    // Decode keypad frame (55-56 bits)
    bool decodeKeypadFrame(volatile unsigned char *bits, unsigned int bitCount, int *keyNumber);

    // Convert binary stream to hex string for debugging
    String binaryToHex(volatile unsigned char *bits, unsigned int bitCount);

    // Get last decoded key
    int getLastKey() const { return lastKeyPressed; }

    // Get last decoded hex pattern
    String getLastHexPattern() const { return lastHexPattern; }

    // Get keypad character for key number
    String getKeyChar(int keyNumber);

private:
    KeypadProcessor();
    ~KeypadProcessor();

    // Prevent copying
    KeypadProcessor(const KeypadProcessor &) = delete;
    KeypadProcessor &operator=(const KeypadProcessor &) = delete;

    int lastKeyPressed;
    String lastHexPattern;
};

extern KeypadProcessor &keypadProcessor;

#endif
