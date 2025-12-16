#include "card_processor.h"
#include "wiegand_interface.h"
#include "net2_interface.h"
#include "reader_manager.h"
#include "gpio_manager.h"
#include "keypad_processor.h"

CardProcessor cardProcessor;

CardProcessor::CardProcessor()
{
    reset();
}

void CardProcessor::reset()
{
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
    cardValid = false;
    isNet2 = false;
    isKeypad = false;
    keypadNumber = -1;

    for (int i = 0; i < MAX_BITS; i++)
    {
        databits[i] = 0;
    }

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

void CardProcessor::processNet2Frame()
{
    noInterrupts();

    unsigned int bc = net2BitCount;
    if (bc > NET2_MAX_BITS)
        bc = NET2_MAX_BITS;

    unsigned char bitsCopy[NET2_MAX_BITS];
    for (unsigned int i = 0; i < bc; i++)
        bitsCopy[i] = net2Bits[i];

    net2FrameActive = false;
    net2BitCount = 0;
    net2FrameReady = false;
    net2LastClockMicros = 0;

    interrupts();

    if (bc >= 55 && bc <= 56)
    {
        int keyNum = -1;

        if (keypadProcessor.decodeKeypadFrame(bitsCopy, bc, &keyNum))
        {
            isKeypad = true;
            isNet2 = false;
            bitCount = bc;
            keypadNumber = keyNum;

            csvHEX = keypadProcessor.getLastHexPattern();

            char binBuf[128];
            for (unsigned int i = 0; i < bc && i < 127; i++)
            {
                binBuf[i] = bitsCopy[i] ? '1' : '0';
            }
            binBuf[bc] = '\0';
            dataStreamBIN = binBuf;

            cardValid = true;
            flagDone = 1;
            return;
        }
    }

    unsigned long cardID = 0;
    unsigned long long fullID = 0;

    if (net2Interface.decode75(bitsCopy, bc, &cardID, &fullID))
    {
        isNet2 = true;
        isKeypad = false;
        bitCount = 75;
        facilityCode = 0;
        cardNumber = cardID;

        char binBuf[76];
        unsigned int binLen = (bc < 75) ? bc : 75;
        for (unsigned int i = 0; i < binLen; i++)
        {
            binBuf[i] = bitsCopy[i] ? '1' : '0';
        }
        binBuf[binLen] = '\0';
        dataStreamBIN = binBuf;

        csvHEX = net2HexEM410x;
        cardValid = true;
        flagDone = 1;
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
    if (readerManager.isPaxtonMode())
    {
        if (net2FrameReady && net2BitCount >= NET2_MIN_BITS)
            {
                processNet2Frame();
            return;
            }

        if (net2BitCount >= NET2_MIN_BITS && net2BitCount > 0)
        {
            unsigned long now = micros();
            if ((now - net2LastClockMicros) > NET2_FRAME_TIMEOUT_US)
            {
                processNet2Frame();
            }
        }
    }
    else
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
            isNet2 = false;
            getDataStream();
            getCardValues();
            getFacilityCodeCardNumber();

            if ((bitCount == 28 || bitCount == 30 || bitCount == 31 || bitCount == 33 ||
                 bitCount == 36 || bitCount == 48) &&
                bitCount != 26 && bitCount != 27 && bitCount != 29 && bitCount != 34 &&
                bitCount != 35 && bitCount != 37 && bitCount != 46 && bitCount != 56)
            {
                csvHEX = String(dataStream, HEX);
                csvHEX.toUpperCase();
            }
            else if (bitCount == 26 || bitCount == 27 || bitCount == 29 || bitCount == 34 ||
                     bitCount == 35 || bitCount == 37 || bitCount == 46 || bitCount == 56)
            {
                csvHEX = String(cardChunk1, HEX) + String(cardChunk2, HEX);
                csvHEX.toUpperCase();
            }
            else if (bitCount == 32)
            {
                if (facilityCode >= 256)
                {
                    pivParse();
                }
                else
                {
                    csvHEX = String(cardChunk1, HEX) + String(cardChunk2, HEX);
                    csvHEX.toUpperCase();
                }
            }
            else
            {
                csvHEX = String(cardChunk1, HEX) + String(cardChunk2, HEX);
                csvHEX.toUpperCase();
            }

            cardValid = true;

            if (cardValid)
            {
                handleGPIOOnCardRead();
            }

            if (bitCount == 4)
            {
                delay(10);
            }
        }
    }
}

bool CardProcessor::isReadComplete() const
{
    if (readerManager.isPaxtonMode())
    {
        return ((bitCount == 75 || (bitCount >= 55 && bitCount <= 56)) && flagDone);
    }
    else
    {
        return (bitCount > 0 && flagDone);
    }
}

String CardProcessor::getBinData() const
{
    return dataStreamBIN;
}

bool CardProcessor::isCardValid() const
{
    return cardValid;
}

String CardProcessor::getCardFormat() const
{
    switch (bitCount)
    {
    case 4:
        return "PIN";
    case 55:
        if (isKeypad)
        {
            return "PIN";
        }
        return "Unknown";
    case 26:
        return "H10301/Ind26/AWID26";
    case 27:
        return "H10307/Ind27";
    case 28:
        return "2804W";
    case 29:
        return "Ind29";
    case 30:
        return "ATSW30";
    case 31:
        return "ADT31";
    case 32:
        if (facilityCode >= 256)
        {
            return "PIV/MiFare/FASC-N";
        }
        return "WIE32/EM";
    case 33:
        return "D10202";
    case 34:
        return "H10306";
    case 35:
        return "C1k35s (C-1000)";
    case 36:
        return "S12906";
    case 37:
        return "H10304";
    case 38:
        return "BQT38/ISCS";
    case 39:
        return "PW39";
    case 40:
        return "P10001/Casi40/Verkada40/BC40/AWID40";
    case 46:
        return "H800002";
    case 48:
        return "C1k48s (C-1000)";
    case 50:
        return "AWID50";
    case 56:
        if (isKeypad)
        {
            return "PIN";
        }
        return "Avig56";
    case 64:
        return "H10309";
    case 75:
        return "Net2/EM";
    default:
        return "Unknown";
    }
}

String CardProcessor::getNet2HexEM410x() const
{
    return net2HexEM410x;
}

String CardProcessor::getNet2HexEM4100() const
{
    return net2HexEM4100;
}

bool CardProcessor::isNet2Card() const
{
    return isNet2;
}

bool CardProcessor::isKeypadPress() const
{
    return isKeypad;
}

int CardProcessor::getKeypadNumber() const
{
    return keypadNumber;
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

unsigned long CardProcessor::readBitsWindow(unsigned char startInclusive, unsigned char endInclusive) const
{
    unsigned long value = 0;
    // Clamp to available range and ensure proper ordering
    if (endInclusive >= MAX_BITS)
        endInclusive = MAX_BITS - 1;
    if (startInclusive > endInclusive)
        return 0;
    for (unsigned char i = startInclusive; i <= endInclusive && i < bitCount; ++i)
    {
        value <<= 1;
        value |= databits[i];
    }
    return value;
}

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

    // HID H800002 46-bit
    case 46:
        for (int i = 19; i >= 0; i--)
        {
            cardChunk1 &= ~(1UL << i);
            if (i == 13 || i == 14)
            {
                cardChunk1 |= 1UL << i;
            }
            else if (i > 14)
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
            if (i >= 14)
            {
                cardChunk2 |= ((bitHolder1 >> (i - 14)) & 1UL) << i;
            }
            else
            {
                cardChunk2 |= ((bitHolder2 >> i) & 1UL) << i;
            }
        }
        break;

    // HID Corporate 1000 48-bit
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

    // Avigilon 56-bit (Avig56)
    case 56:
        for (int i = 19; i >= 0; i--)
        {
            cardChunk1 &= ~(1UL << i);
            if (i == 13 || i == 17)
            {
                cardChunk1 |= 1UL << i;
            }
            else if (i > 17)
            {
                // Keep bit cleared
            }
            else
            {
                cardChunk1 |= ((bitHolder1 >> (i + 25)) & 1UL) << i;
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

    // HID H800002 46-bit
    case 46:
        for (i = 1; i < 15; i++)
        {
            facilityCode <<= 1;
            facilityCode |= databits[i];
        }

        for (i = 15; i < 45; i++)
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

    // AWID 50-bit
    case 50:
        for (i = 1; i < 17; i++) // FC: bits 1-16
        {
            facilityCode <<= 1;
            facilityCode |= databits[i];
        }
        for (i = 17; i < 49; i++) // CN: bits 17-48
        {
            cardNumber <<= 1;
            cardNumber |= databits[i];
        }
        break;

    // Avigilon 56-bit (Avig56)
    case 56:
        // From debug windows: readBitsWindow(1,32) forms 0x0022B000 for FC=555, so shift by 12
        facilityCode = (unsigned long)(readBitsWindow(1, 32) >> 12);
        // CN observed stable in bits 33..54 in logs
        cardNumber = readBitsWindow(33, 54);
        break;

    // Net2 cards
    case 75:
        break;
    }
}