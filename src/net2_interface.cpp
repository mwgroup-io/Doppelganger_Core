#include "net2_interface.h"

// Net2/Paxton reader support - handles 75-bit Net2 protocol
volatile unsigned char net2Bits[NET2_MAX_BITS];
volatile unsigned int net2BitCount = 0;
volatile unsigned long net2LastClockMicros = 0;
volatile bool net2FrameActive = false;
volatile bool net2FrameReady = false;

String net2HexEM410x = "";
String net2HexEM4100 = "";

Net2Interface &net2Interface = Net2Interface::getInstance();

Net2Interface::Net2Interface() {}

Net2Interface::~Net2Interface() {}

Net2Interface &Net2Interface::getInstance()
{
    static Net2Interface instance;
    return instance;
}

void Net2Interface::begin()
{
    pinMode(NET2_CLK_PIN, INPUT);
    pinMode(NET2_DATA_PIN, INPUT);
    attachInterrupt(NET2_CLK_PIN, ISR_NET2_CLK_FALLING, FALLING);
    reset();
}

void Net2Interface::reset()
{
    net2BitCount = 0;
    net2LastClockMicros = 0;
    net2FrameActive = false;
    net2FrameReady = false;
    net2HexEM410x = "";
    net2HexEM4100 = "";
}

void Net2Interface::processTimeout()
{
    if (net2FrameActive && !net2FrameReady)
    {
        unsigned long now = micros();
        if ((now - net2LastClockMicros) > NET2_FRAME_TIMEOUT_US)
        {
            if (net2BitCount >= NET2_MIN_BITS)
            {
                net2FrameReady = true;
            }
            else
            {
                net2FrameActive = false;
                net2BitCount = 0;
            }
        }
    }
}

void IRAM_ATTR ISR_NET2_CLK_FALLING()
{
    unsigned long now = micros();

    if ((now - net2LastClockMicros) < NET2_MIN_CLK_US)
        return;

    if (net2FrameReady)
        return;

    net2FrameActive = true;
    net2LastClockMicros = now;

    int bitVal = digitalRead(NET2_DATA_PIN) ? 0 : 1;

    if (net2BitCount < NET2_MAX_BITS)
    {
        net2Bits[net2BitCount++] = (unsigned char)bitVal;
    }

    if (net2BitCount >= 75)
    {
        net2FrameReady = true;
    }
}

bool Net2Interface::decode75(volatile unsigned char *bits, unsigned int bitCount,
                             unsigned long *cardID, unsigned long long *fullID)
{
    if (bitCount != 75)
        return false;

    for (int i = 0; i < 10; i++)
    {
        if (bits[i] != 0)
            return false;
    }

    for (int i = 65; i < 75; i++)
    {
        if (bits[i] != 0)
            return false;
    }

    int nibbles[11];
    int lrc[4] = {0, 0, 0, 0};
    String cardNumberStr = "";

    for (int nibbleIdx = 0; nibbleIdx < 11; nibbleIdx++)
    {
        int bitPos = 10 + (nibbleIdx * 5);

        int b0 = bits[bitPos + 0];
        int b1 = bits[bitPos + 1];
        int b2 = bits[bitPos + 2];
        int b3 = bits[bitPos + 3];
        int b4 = bits[bitPos + 4];

        int nibbleVal = (8 * b3) + (4 * b2) + (2 * b1) + (1 * b0);
        nibbles[nibbleIdx] = nibbleVal;

        int rowSum = b0 + b1 + b2 + b3;
        int expectedParity = (rowSum % 2 == 0) ? 1 : 0;

        if (expectedParity != b4)
            return false;

        if (nibbleIdx < 10)
        {
            lrc[0] += b0;
            lrc[1] += b1;
            lrc[2] += b2;
            lrc[3] += b3;
        }
    }

    if (nibbles[0] != 11)
        return false;

    if (nibbles[9] != 15)
        return false;

    int lrcExpected = ((lrc[0] % 2 == 0) ? 0 : 1) |
                      (((lrc[1] % 2 == 0) ? 0 : 1) << 1) |
                      (((lrc[2] % 2 == 0) ? 0 : 1) << 2) |
                      (((lrc[3] % 2 == 0) ? 0 : 1) << 3);

    if (lrcExpected != nibbles[10])
        return false;

    for (int i = 1; i <= 8; i++)
    {
        cardNumberStr += String(nibbles[i]);
    }

    unsigned long cardNum = cardNumberStr.toInt();
    *cardID = cardNum;

    // Compute EM410x chip ID
    unsigned long long chipID = 0;
    if (cardNum >= 14000000UL && cardNum <= 15000000UL)
    {
        chipID = 0x0F0BEBC200ULL + (unsigned long long)cardNum;
    }
    else
    {
        chipID = (unsigned long long)cardNum;
    }

    unsigned long cardChunk1 = (chipID >> 32) & 0xFF;
    unsigned long cardChunk2 = (unsigned long)(chipID & 0xFFFFFFFFUL);

    net2HexEM410x = "";
    if (cardNum >= 14000000UL && cardNum <= 15000000UL)
    {
        if (cardChunk1 < 0x10)
            net2HexEM410x += "0";
        net2HexEM410x += String(cardChunk1, HEX);
        String low = String(cardChunk2, HEX);
        for (int p = 0; p < (8 - (int)low.length()); p++)
            net2HexEM410x += "0";
        net2HexEM410x += low;
        net2HexEM410x.toUpperCase();
    }

    net2HexEM4100 = String(cardNum, HEX);
    for (int p = 0; p < (8 - (int)net2HexEM4100.length()); p++)
        net2HexEM4100 = "0" + net2HexEM4100;
    net2HexEM4100.toUpperCase();

    if (net2HexEM410x.length() == 0)
    {
        net2HexEM410x = net2HexEM4100;
        while ((int)net2HexEM410x.length() < 10)
            net2HexEM410x = "0" + net2HexEM410x;
        net2HexEM410x.toUpperCase();
    }

    if (fullID != nullptr)
    {
        unsigned long long rawFrame = 0;
        for (int i = 0; i < 10; i++)
        {
            rawFrame = (rawFrame << 4) | nibbles[i];
        }
        *fullID = rawFrame;
    }

    return true;
}
