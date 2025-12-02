#ifndef NET2_INTERFACE_H
#define NET2_INTERFACE_H

#include <Arduino.h>
#include "version_config.h"

// Net2/Paxton reader support - handles 75-bit Net2 protocol

// Net2 configuration
#define NET2_SAMPLE_RISING 0 // 0 = FALLING edge, 1 = RISING edge
#define NET2_INVERT_DATA 0   // 1 = invert sampled data bit
#define NET2_MIN_CLK_US 20   // ignore clocks closer than this (us)

// Net2 pin assignments (D1=CLK, D0=DATA)
#define NET2_CLK_PIN DATA1  // D1 is clock
#define NET2_DATA_PIN DATA0 // D0 is data

// Net2 buffer sizes
#define NET2_MAX_BITS 256
#define NET2_MIN_BITS 50            // Ignore partial captures
#define NET2_FRAME_TIMEOUT_US 30000 // 30ms frame timeout

// Forward declaration of ISR
void IRAM_ATTR ISR_NET2_CLK_FALLING();

// Net2 capture buffers
extern volatile unsigned char net2Bits[NET2_MAX_BITS];
extern volatile unsigned int net2BitCount;
extern volatile unsigned long net2LastClockMicros;
extern volatile bool net2FrameActive;
extern volatile bool net2FrameReady;

// Net2 hex strings
extern String net2HexEM410x;
extern String net2HexEM4100;

class Net2Interface
{
public:
    static Net2Interface &getInstance();
    void begin();
    void reset();
    bool isFrameReady() const { return net2FrameReady; }
    unsigned int getBitCount() const { return net2BitCount; }

    // Decode Net2 75-bit frame
    bool decode75(volatile unsigned char *bits, unsigned int bitCount,
                  unsigned long *cardID, unsigned long long *fullID = nullptr);

    // Process Net2 frame timeout in main loop
    void processTimeout();

private:
    Net2Interface();
    ~Net2Interface();

    // Prevent copying
    Net2Interface(const Net2Interface &) = delete;
    Net2Interface &operator=(const Net2Interface &) = delete;
};

extern Net2Interface &net2Interface;

#endif
