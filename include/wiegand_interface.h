#ifndef WIEGAND_INTERFACE_H
#define WIEGAND_INTERFACE_H

#include <Arduino.h>
#include "card_processor.h"
#include "version_config.h" // For pin definitions

// Forward declarations of interrupt handlers
void IRAM_ATTR handleData0();
void IRAM_ATTR handleData1();

// Wiegand interface buffers
extern unsigned char databits[MAX_BITS];
extern volatile unsigned int bitCount;
extern unsigned char flagDone;
extern unsigned int weigand_counter;

// Card data storage
extern volatile unsigned long facilityCode;
extern volatile unsigned long cardNumber;
extern volatile unsigned long dataStream;

// HEX conversion buffers
extern volatile unsigned long bitHolder1;
extern volatile unsigned long bitHolder2;
extern volatile unsigned long cardChunk1;
extern volatile unsigned long cardChunk2;

// PIV/MF card data buffers
extern volatile unsigned long pivUID;
extern volatile unsigned long chunk1Hex;
extern volatile unsigned long chunk2Hex;
extern volatile unsigned long paddedChunk2Hex;
extern volatile unsigned long reverseUID;
extern String reversedPairsUID;
extern String csvHEX;
extern String dataStreamBIN;

class WiegandInterface
{
public:
    static WiegandInterface &getInstance();
    void begin();
    void reset();
    bool isReadComplete() const { return flagDone; }
    unsigned int getBitCount() const { return bitCount; }
    unsigned long getFacilityCode() const { return facilityCode; }
    unsigned long getCardNumber() const { return cardNumber; }
    unsigned long getDataStream() const { return dataStream; }
    String getBinData() const { return dataStreamBIN; }
    String getCsvHEX() const { return csvHEX; }
    String getReversedPairsUID() const { return reversedPairsUID; }

private:
    WiegandInterface();
    ~WiegandInterface();

    // Prevent copying
    WiegandInterface(const WiegandInterface &) = delete;
    WiegandInterface &operator=(const WiegandInterface &) = delete;
};

extern WiegandInterface &wiegandInterface;

#endif