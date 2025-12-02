#ifndef CARD_PROCESSOR_H
#define CARD_PROCESSOR_H

#include <Arduino.h>
#include "version_config.h"

// Forward declaration - gpio_manager included in .cpp
class GPIOManager;

class CardProcessor
{
public:
    CardProcessor();

    // ISR handlers for HID mode
    void ISR_INT0(); // Handle DATA0 interrupt
    void ISR_INT1(); // Handle DATA1 interrupt

    void processCard();
    bool isReadComplete() const;
    void reset();

    // GPIO control
    void setPin35OnCardRead(bool enable);
    void setPin36OnCardRead(bool enable);
    bool isPin35OnCardRead() const;
    bool isPin36OnCardRead() const;

    // Card data getters
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

    // Net2/Paxton card data
    String getNet2HexEM410x() const;
    String getNet2HexEM4100() const;
    bool isNet2Card() const;

    // Keypad data
    bool isKeypadPress() const;
    int getKeypadNumber() const;

    // Card format identification
    String getCardFormat() const;

    // Debug helpers
    unsigned long readBitsWindow(unsigned char startInclusive, unsigned char endInclusive) const;

    // Public card data members (accessed by ISRs and processing functions)

    // Wiegand buffers
    unsigned char databits[MAX_BITS];
    volatile unsigned int bitCount;
    unsigned char flagDone;
    unsigned int weigand_counter;

    // Card data
    volatile unsigned long facilityCode;
    volatile unsigned long cardNumber;
    volatile unsigned long dataStream;

    // HEX conversion buffers
    volatile unsigned long bitHolder1;
    volatile unsigned long bitHolder2;
    volatile unsigned long cardChunk1;
    volatile unsigned long cardChunk2;

    // PIV/MF card buffers
    volatile unsigned long pivUID;
    volatile unsigned long chunk1Hex;
    volatile unsigned long chunk2Hex;
    volatile unsigned long paddedChunk2Hex;
    volatile unsigned long reverseUID;
    String reversedPairsUID;
    String csvHEX;
    String dataStreamBIN;

private:
    // HID card processing
    void getDataStream();
    void getCardValues();
    void getFacilityCodeCardNumber();
    void pivParse();
    void handleGPIOOnCardRead();
    void processNet2Frame();

    // State flags
    bool pin35OnCardRead;
    bool pin36OnCardRead;
    bool cardValid;
    bool isNet2;
    bool isKeypad;
    int keypadNumber;
};

// Global instance declaration
extern CardProcessor cardProcessor;

#endif