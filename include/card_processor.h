#ifndef CARD_PROCESSOR_H
#define CARD_PROCESSOR_H

#include <Arduino.h>
#include "gpio_manager.h"

// Constants
#define MAX_BITS 100          // Max read bits
#define WEIGAND_WAIT_TIME 400 // ms between pulses

class CardProcessor
{
public:
    CardProcessor();
    void ISR_INT0();    // Handle DATA0 interrupt
    void ISR_INT1();    // Handle DATA1 interrupt
    void processCard(); // Main processing function
    bool isReadComplete() const;
    void reset(); // Reset all card data

    // GPIO Control Methods
    void setPin35OnCardRead(bool enable);
    void setPin36OnCardRead(bool enable);
    bool isPin35OnCardRead() const;
    bool isPin36OnCardRead() const;

    // Getters for card data
    unsigned long getFacilityCode() const;
    unsigned long getCardNumber() const;
    String getDataStreamBIN() const;
    String getReversedPairsUID() const;
    String getCsvHEX() const;
    unsigned int getBitCount() const;
    unsigned long getBitHolder1() const;
    unsigned long getCardChunk1() const;
    unsigned long getDataStream() const;
    String getBinData() const;
    bool isCardValid() const;

private:
    void getDataStream();
    void getCardValues();
    void getFacilityCodeCardNumber();
    void pivParse();
    void handleGPIOOnCardRead();

    // Card data storage
    unsigned char databits[MAX_BITS];
    volatile unsigned int bitCount;
    unsigned char flagDone;
    unsigned int weigand_counter;

    // GPIO control flags
    bool pin35OnCardRead;
    bool pin36OnCardRead;

    // Card values
    volatile unsigned long facilityCode;
    volatile unsigned long cardNumber;
    volatile unsigned long dataStream;
    volatile unsigned long bitHolder1;
    volatile unsigned long bitHolder2;
    volatile unsigned long cardChunk1;
    volatile unsigned long cardChunk2;

    // PIV/MF specific data
    volatile unsigned long pivUID;
    volatile unsigned long chunk1Hex;
    volatile unsigned long chunk2Hex;
    volatile unsigned long paddedChunk2Hex;
    volatile unsigned long reverseUID;
    String reversedPairsUID;
    String csvHEX;
    String dataStreamBIN;
    String binData;
    bool cardValid;
};

// Global instance declaration
extern CardProcessor cardProcessor;

#endif