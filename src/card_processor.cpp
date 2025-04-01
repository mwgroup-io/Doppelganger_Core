#include "card_processor.h"

// Global instance definition - must be before any other code
CardProcessor cardProcessor;

CardProcessor::CardProcessor()
{
    reset();
}

void CardProcessor::reset()
{
    // Save current GPIO settings
    bool savedPin35 = pin35OnCardRead;
    bool savedPin36 = pin36OnCardRead;

    bitCount = 0;
    facilityCode = 0;
    cardNumber = 0;
    bitHolder1 = 0;
    bitHolder2 = 0;
    cardChunk1 = 0;
    cardChunk2 = 0;
    chunk1Hex = 0;
    chunk2Hex = 0;
    paddedChunk2Hex = 0;
    reverseUID = 0;
    pivUID = 0;
    reversedPairsUID = "";
    csvHEX = "";
    dataStream = 0;
    dataStreamBIN = "";
    flagDone = 0;
    weigand_counter = WEIGAND_WAIT_TIME;
    binData = "";
    cardValid = false;

    for (int i = 0; i < MAX_BITS; i++)
    {
        databits[i] = 0;
    }

    // Restore GPIO settings
    pin35OnCardRead = savedPin35;
    pin36OnCardRead = savedPin36;
}

// GPIO Control Methods
void CardProcessor::setPin35OnCardRead(bool enable)
{
    pin35OnCardRead = enable;
}

void CardProcessor::setPin36OnCardRead(bool enable)
{
    pin36OnCardRead = enable;
}

bool CardProcessor::isPin35OnCardRead() const
{
    return pin35OnCardRead;
}

bool CardProcessor::isPin36OnCardRead() const
{
    return pin36OnCardRead;
}

void CardProcessor::handleGPIOOnCardRead()
{
    if (pin35OnCardRead)
    {
        GPIOManager::getInstance().pulsePin35();
    }
    if (pin36OnCardRead)
    {
        GPIOManager::getInstance().pulsePin36();
    }
}

void CardProcessor::ISR_INT0()
{
    bitCount++;
    flagDone = 0;

    if (bitCount < 23)
    {
        bitHolder1 = bitHolder1 << 1;
    }
    else
    {
        bitHolder2 = bitHolder2 << 1;
    }

    weigand_counter = WEIGAND_WAIT_TIME;
}

void CardProcessor::ISR_INT1()
{
    databits[bitCount] = 1;
    bitCount++;
    flagDone = 0;

    if (bitCount < 23)
    {
        bitHolder1 = bitHolder1 << 1;
        bitHolder1 |= 1;
    }
    else
    {
        bitHolder2 = bitHolder2 << 1;
        bitHolder2 |= 1;
    }

    weigand_counter = WEIGAND_WAIT_TIME;
}

void CardProcessor::getDataStream()
{
    unsigned char i;

    for (i = 0; i < bitCount; i++)
    {
        dataStream <<= 1;
        dataStream |= databits[i];
    }

    dataStreamBIN = String(dataStream, BIN);

    while (dataStreamBIN.length() < bitCount)
    {
        dataStreamBIN = "0" + dataStreamBIN;
    }
}

void CardProcessor::pivParse()
{
    char chunk1Hex[6];
    sprintf(chunk1Hex, "%05lX", cardChunk1);

    char chunk2Hex[7];
    sprintf(chunk2Hex, "%05lX", cardChunk2);

    if (strlen(chunk2Hex) < 6)
    {
        char paddedChunk2Hex[7];
        snprintf(paddedChunk2Hex, 7, "0%s", chunk2Hex);
        strncpy(chunk2Hex, paddedChunk2Hex, 7);
    }

    String reverseUID = String(chunk1Hex) + String(chunk2Hex);
    String pivUID = reverseUID.substring(2);
    for (int i = pivUID.length() - 2; i >= 0; i -= 2)
    {
        reversedPairsUID += pivUID.substring(i, i + 2);
    }

    String combinedHex = String(cardChunk1, HEX) + String(chunk2Hex);
    combinedHex.toUpperCase();
    csvHEX = combinedHex;
}

void CardProcessor::processCard()
{
    if (!flagDone)
    {
        if (--weigand_counter == 0)
        {
            flagDone = 1;
        }
    }

    if (bitCount > 0 && flagDone)
    {
        getDataStream();
        getCardValues();
        getFacilityCodeCardNumber();
        if (bitCount == 32 && facilityCode >= 256)
        {
            pivParse();
        }

        // Store BIN data
        binData = dataStreamBIN;
        cardValid = true;

        // Handle GPIO control on valid card read
        if (cardValid)
        {
            handleGPIOOnCardRead();
        }

        // For PIN entries, add a small delay to ensure reliable processing
        if (bitCount == 4)
        {
            delay(10); // Small delay for PIN entries
        }
    }
}

bool CardProcessor::isReadComplete() const
{
    return (bitCount > 0 && flagDone);
}

String CardProcessor::getBinData() const
{
    return binData;
}

bool CardProcessor::isCardValid() const
{
    return cardValid;
}

// Getters
unsigned long CardProcessor::getFacilityCode() const { return facilityCode; }
unsigned long CardProcessor::getCardNumber() const { return cardNumber; }
String CardProcessor::getDataStreamBIN() const { return dataStreamBIN; }
String CardProcessor::getReversedPairsUID() const { return reversedPairsUID; }
String CardProcessor::getCsvHEX() const { return csvHEX; }
unsigned int CardProcessor::getBitCount() const { return bitCount; }
unsigned long CardProcessor::getBitHolder1() const { return bitHolder1; }
unsigned long CardProcessor::getCardChunk1() const { return cardChunk1; }
unsigned long CardProcessor::getDataStream() const { return dataStream; }

void CardProcessor::getCardValues()
{
    switch (bitCount)
    {
    case 4:
        for (int i = 4; i >= 0; i--)
        {
            // Clear target bit
            cardChunk1 &= ~(1UL << i);
            cardChunk2 &= ~(1UL << i);
            // Set target bit
            cardChunk1 |= ((bitHolder1 >> i) & 1UL) << i;
            cardChunk2 |= ((bitHolder2 >> i) & 1UL) << i;
        }
        break;

    case 26:
        for (int i = 19; i >= 0; i--)
        {
            cardChunk1 &= ~(1UL << i);
            if (i == 13 || i == 2)
            {
                cardChunk1 |= 1UL << i;
            }
            else if (i > 2)
            {
                // Keep bit cleared
            }
            else
            {
                cardChunk1 |= ((bitHolder1 >> (i + 20)) & 1UL) << i;
            }
        }
        for (int i = 23; i >= 0; i--)
        {
            cardChunk2 &= ~(1UL << i);
            if (i >= 4)
            {
                cardChunk2 |= ((bitHolder1 >> (i - 4)) & 1UL) << i;
            }
            else
            {
                cardChunk2 |= ((bitHolder2 >> i) & 1UL) << i;
            }
        }
        break;

    case 27:
        for (int i = 19; i >= 0; i--)
        {
            cardChunk1 &= ~(1UL << i);
            if (i == 13 || i == 3)
            {
                cardChunk1 |= 1UL << i;
            }
            else if (i > 3)
            {
                // Keep bit cleared
            }
            else
            {
                cardChunk1 |= ((bitHolder1 >> (i + 19)) & 1UL) << i;
            }
        }
        for (int i = 23; i >= 0; i--)
        {
            cardChunk2 &= ~(1UL << i);
            if (i >= 5)
            {
                cardChunk2 |= ((bitHolder1 >> (i - 5)) & 1UL) << i;
            }
            else
            {
                cardChunk2 |= ((bitHolder2 >> i) & 1UL) << i;
            }
        }
        break;

    case 28:
        for (int i = 19; i >= 0; i--)
        {
            cardChunk1 &= ~(1UL << i);
            if (i == 13 || i == 4)
            {
                cardChunk1 |= 1UL << i;
            }
            else if (i > 4)
            {
                // Keep bit cleared
            }
            else
            {
                cardChunk1 |= ((bitHolder1 >> (i + 18)) & 1UL) << i;
            }
        }
        for (int i = 23; i >= 0; i--)
        {
            cardChunk2 &= ~(1UL << i);
            if (i >= 6)
            {
                cardChunk2 |= ((bitHolder1 >> (i - 6)) & 1UL) << i;
            }
            else
            {
                cardChunk2 |= ((bitHolder2 >> i) & 1UL) << i;
            }
        }
        break;

    case 29:
        for (int i = 19; i >= 0; i--)
        {
            cardChunk1 &= ~(1UL << i);
            if (i == 13 || i == 5)
            {
                cardChunk1 |= 1UL << i;
            }
            else if (i > 5)
            {
                // Keep bit cleared
            }
            else
            {
                cardChunk1 |= ((bitHolder1 >> (i + 17)) & 1UL) << i;
            }
        }
        for (int i = 23; i >= 0; i--)
        {
            cardChunk2 &= ~(1UL << i);
            if (i >= 7)
            {
                cardChunk2 |= ((bitHolder1 >> (i - 7)) & 1UL) << i;
            }
            else
            {
                cardChunk2 |= ((bitHolder2 >> i) & 1UL) << i;
            }
        }
        break;

    case 30:
        for (int i = 19; i >= 0; i--)
        {
            cardChunk1 &= ~(1UL << i);
            if (i == 13 || i == 6)
            {
                cardChunk1 |= 1UL << i;
            }
            else if (i > 6)
            {
                // Keep bit cleared
            }
            else
            {
                cardChunk1 |= ((bitHolder1 >> (i + 16)) & 1UL) << i;
            }
        }
        for (int i = 23; i >= 0; i--)
        {
            cardChunk2 &= ~(1UL << i);
            if (i >= 8)
            {
                cardChunk2 |= ((bitHolder1 >> (i - 8)) & 1UL) << i;
            }
            else
            {
                cardChunk2 |= ((bitHolder2 >> i) & 1UL) << i;
            }
        }
        break;

    case 31:
        for (int i = 19; i >= 0; i--)
        {
            cardChunk1 &= ~(1UL << i);
            if (i == 13 || i == 7)
            {
                cardChunk1 |= 1UL << i;
            }
            else if (i > 7)
            {
                // Keep bit cleared
            }
            else
            {
                cardChunk1 |= ((bitHolder1 >> (i + 15)) & 1UL) << i;
            }
        }
        for (int i = 23; i >= 0; i--)
        {
            cardChunk2 &= ~(1UL << i);
            if (i >= 9)
            {
                cardChunk2 |= ((bitHolder1 >> (i - 9)) & 1UL) << i;
            }
            else
            {
                cardChunk2 |= ((bitHolder2 >> i) & 1UL) << i;
            }
        }
        break;

    case 32:
        for (int i = 19; i >= 0; i--)
        {
            cardChunk1 &= ~(1UL << i);
            if (i == 13 || i == 8)
            {
                cardChunk1 |= 1UL << i;
            }
            else if (i > 8)
            {
                // Keep bit cleared
            }
            else
            {
                cardChunk1 |= ((bitHolder1 >> (i + 14)) & 1UL) << i;
            }
        }
        for (int i = 23; i >= 0; i--)
        {
            cardChunk2 &= ~(1UL << i);
            if (i >= 10)
            {
                cardChunk2 |= ((bitHolder1 >> (i - 10)) & 1UL) << i;
            }
            else
            {
                cardChunk2 |= ((bitHolder2 >> i) & 1UL) << i;
            }
        }
        break;

    case 33:
        for (int i = 19; i >= 0; i--)
        {
            cardChunk1 &= ~(1UL << i);
            if (i == 15 || i == 11)
            {
                cardChunk1 |= 1UL << i;
            }
            else if (i > 11)
            {
                // Keep bit cleared
            }
            else
            {
                cardChunk1 |= ((bitHolder1 >> (i + 17)) & 1UL) << i;
            }
        }
        for (int i = 23; i >= 0; i--)
        {
            cardChunk2 &= ~(1UL << i);
            if (i >= 15)
            {
                cardChunk2 |= ((bitHolder1 >> (i - 15)) & 1UL) << i;
            }
            else
            {
                cardChunk2 |= ((bitHolder2 >> i) & 1UL) << i;
            }
        }
        break;

    case 34:
        for (int i = 19; i >= 0; i--)
        {
            cardChunk1 &= ~(1UL << i);
            if (i == 13 || i == 10)
            {
                cardChunk1 |= 1UL << i;
            }
            else if (i > 10)
            {
                // Keep bit cleared
            }
            else
            {
                cardChunk1 |= ((bitHolder1 >> (i + 12)) & 1UL) << i;
            }
        }
        for (int i = 23; i >= 0; i--)
        {
            cardChunk2 &= ~(1UL << i);
            if (i >= 12)
            {
                cardChunk2 |= ((bitHolder1 >> (i - 12)) & 1UL) << i;
            }
            else
            {
                cardChunk2 |= ((bitHolder2 >> i) & 1UL) << i;
            }
        }
        break;

    case 35:
        for (int i = 19; i >= 0; i--)
        {
            cardChunk1 &= ~(1UL << i);
            if (i == 13 || i == 11)
            {
                cardChunk1 |= 1UL << i;
            }
            else if (i > 11)
            {
                // Keep bit cleared
            }
            else
            {
                cardChunk1 |= ((bitHolder1 >> (i + 11)) & 1UL) << i;
            }
        }
        for (int i = 23; i >= 0; i--)
        {
            cardChunk2 &= ~(1UL << i);
            if (i >= 13)
            {
                cardChunk2 |= ((bitHolder1 >> (i - 13)) & 1UL) << i;
            }
            else
            {
                cardChunk2 |= ((bitHolder2 >> i) & 1UL) << i;
            }
        }
        break;

    case 36:
        for (int i = 19; i >= 0; i--)
        {
            cardChunk1 &= ~(1UL << i);
            if (i == 17 || i == 16)
            {
                cardChunk1 |= 1UL << i;
            }
            else if (i > 16)
            {
                // Keep bit cleared
            }
            else
            {
                cardChunk1 |= ((bitHolder1 >> (i + 14)) & 1UL) << i;
            }
        }
        for (int i = 23; i >= 0; i--)
        {
            cardChunk2 &= ~(1UL << i);
            if (i >= 18)
            {
                cardChunk2 |= ((bitHolder1 >> (i - 18)) & 1UL) << i;
            }
            else
            {
                cardChunk2 |= ((bitHolder2 >> i) & 1UL) << i;
            }
        }
        break;

    case 37:
        for (int i = 19; i >= 0; i--)
        {
            cardChunk1 &= ~(1UL << i);
            if (i == 13)
            {
                // Keep bit cleared
            }
            else
            {
                cardChunk1 |= ((bitHolder1 >> (i + 9)) & 1UL) << i;
            }
        }
        for (int i = 23; i >= 0; i--)
        {
            cardChunk2 &= ~(1UL << i);
            if (i >= 15)
            {
                cardChunk2 |= ((bitHolder1 >> (i - 15)) & 1UL) << i;
            }
            else
            {
                cardChunk2 |= ((bitHolder2 >> i) & 1UL) << i;
            }
        }
        break;

    case 48:
        for (int i = 19; i >= 0; i--)
        {
            cardChunk1 &= ~(1UL << i);
            if (i == 13 || i == 15)
            {
                cardChunk1 |= 1UL << i;
            }
            else if (i > 15)
            {
                // Keep bit cleared
            }
            else
            {
                cardChunk1 |= ((bitHolder1 >> (i + 19)) & 1UL) << i;
            }
        }
        for (int i = 23; i >= 0; i--)
        {
            cardChunk2 &= ~(1UL << i);
            if (i >= 16)
            {
                cardChunk2 |= ((bitHolder1 >> (i - 16)) & 1UL) << i;
            }
            else
            {
                cardChunk2 |= ((bitHolder2 >> i) & 1UL) << i;
            }
        }
        break;
    }
}

void CardProcessor::getFacilityCodeCardNumber()
{
    unsigned char i;

    switch (bitCount)
    {
    // Standard HID H10301 26-bit / Indala 26-bit
    case 26:
        for (i = 1; i < 9; i++)
        {
            facilityCode <<= 1;
            facilityCode |= databits[i];
        }

        for (i = 9; i < 25; i++)
        {
            cardNumber <<= 1;
            cardNumber |= databits[i];
        }
        break;

    // Indala 27-bit
    case 27:
        for (i = 1; i < 13; i++)
        {
            facilityCode <<= 1;
            facilityCode |= databits[i];
        }

        for (i = 14; i < 27; i++)
        {
            cardNumber <<= 1;
            cardNumber |= databits[i];
        }
        break;

    // 2804 WIEGAND 28-bit
    case 28:
        for (i = 4; i < 12; i++)
        {
            facilityCode <<= 1;
            facilityCode |= databits[i];
        }

        for (i = 13; i < 27; i++)
        {
            cardNumber <<= 1;
            cardNumber |= databits[i];
        }
        break;

    // Indala 29-bit
    case 29:
        for (i = 1; i < 13; i++)
        {
            facilityCode <<= 1;
            facilityCode |= databits[i];
        }

        for (i = 14; i < 29; i++)
        {
            cardNumber <<= 1;
            cardNumber |= databits[i];
        }
        break;

    // ATS Wiegand 30-bit
    case 30:
        for (i = 2; i < 13; i++)
        {
            facilityCode <<= 1;
            facilityCode |= databits[i];
        }

        for (i = 14; i < 29; i++)
        {
            cardNumber <<= 1;
            cardNumber |= databits[i];
        }
        break;

    // HID ADT 31-Bit
    case 31:
        for (i = 1; i < 5; i++)
        {
            facilityCode <<= 1;
            facilityCode |= databits[i];
        }

        for (i = 5; i < 28; i++)
        {
            cardNumber <<= 1;
            cardNumber |= databits[i];
        }
        break;

    // WEI32 (EM4102)
    case 32:
        for (i = 1; i < 16; i++)
        {
            facilityCode <<= 1;
            facilityCode |= databits[i];
        }

        for (i = 16; i < 32; i++)
        {
            cardNumber <<= 1;
            cardNumber |= databits[i];
        }
        break;

    // HID D10202 33-bit
    case 33:
        for (i = 1; i < 8; i++)
        {
            facilityCode <<= 1;
            facilityCode |= databits[i];
        }

        for (i = 8; i < 32; i++)
        {
            cardNumber <<= 1;
            cardNumber |= databits[i];
        }
        break;

    // HID H10306 34-bit
    case 34:
        for (i = 1; i < 17; i++)
        {
            facilityCode <<= 1;
            facilityCode |= databits[i];
        }

        for (i = 17; i < 33; i++)
        {
            cardNumber <<= 1;
            cardNumber |= databits[i];
        }
        break;

    // HID Corporate 1000 35-bit
    case 35:
        for (i = 2; i < 14; i++)
        {
            facilityCode <<= 1;
            facilityCode |= databits[i];
        }

        for (i = 14; i < 34; i++)
        {
            cardNumber <<= 1;
            cardNumber |= databits[i];
        }
        break;

    // HID Simplex 36-bit (S12906)
    case 36:
        for (i = 1; i <= 8; i++)
        {
            facilityCode <<= 1;
            facilityCode |= databits[i];
        }

        for (i = 19; i < 35; i++)
        {
            cardNumber <<= 1;
            cardNumber |= databits[i];
        }
        break;

    // HID H10304 37-bit
    case 37:
        for (i = 1; i < 17; i++)
        {
            facilityCode <<= 1;
            facilityCode |= databits[i];
        }

        for (i = 17; i < 36; i++)
        {
            cardNumber <<= 1;
            cardNumber |= databits[i];
        }
        break;

    // HID Corporate 1000 48-bit
    case 48:
        for (i = 2; i < 24; i++)
        {
            facilityCode <<= 1;
            facilityCode |= databits[i];
        }

        for (i = 24; i < 47; i++)
        {
            cardNumber <<= 1;
            cardNumber |= databits[i];
        }
        break;
    }
}