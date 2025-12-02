#include "keypad_processor.h"

KeypadProcessor &keypadProcessor = KeypadProcessor::getInstance();

KeypadProcessor::KeypadProcessor()
    : lastKeyPressed(-1)
{
}

KeypadProcessor::~KeypadProcessor() {}

KeypadProcessor &KeypadProcessor::getInstance()
{
    static KeypadProcessor instance;
    return instance;
}

String KeypadProcessor::binaryToHex(volatile unsigned char *bits, unsigned int bitCount)
{
    String hexStr = "";

    for (unsigned int i = 0; i < bitCount; i += 8)
    {
        unsigned char byte = 0;
        for (int j = 0; j < 8 && (i + j) < bitCount; j++)
        {
            byte = (byte << 1) | bits[i + j];
        }

        char hexBuf[3];
        sprintf(hexBuf, "%02X", byte);
        hexStr += hexBuf;
    }

    return hexStr;
}

bool KeypadProcessor::decodeKeypadFrame(volatile unsigned char *bits, unsigned int bitCount, int *keyNumber)
{
    // Validate bit count for keypad data
    if (bitCount < KEYPAD_MIN_BITS || bitCount > KEYPAD_MAX_BITS)
    {
        return false;
    }

    int firstOne = -1;
    int lastOne = -1;

    for (unsigned int i = 0; i < bitCount; i++)
    {
        if (bits[i] == 1)
        {
            if (firstOne == -1)
                firstOne = i;
            lastOne = i;
        }
    }

    // Decode pattern from trimmed data
    if (firstOne >= 0 && lastOne >= 0)
    {
        int dataLength = lastOne - firstOne + 1;

        String dataHex = "";
        for (int i = 0; i < dataLength; i += 4)
        {
            int nibble = 0;
            for (int j = 0; j < 4 && (i + j) < dataLength; j++)
            {
                nibble = (nibble << 1) | bits[firstOne + i + j];
            }
            char hexChar = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
            dataHex += hexChar;
        }

        // Check for release pattern first (ignore it)
        if (dataHex.startsWith("D1E737"))
        {
            return false;
        }

        int detectedKey = -1;

        if (dataHex.startsWith("D1C337"))
        {
            detectedKey = 0;
        }
        else if (dataHex.startsWith("D1C21"))
        {
            detectedKey = 1;
        }
        else if (dataHex.startsWith("D1C20"))
        {
            detectedKey = 1;
        }
        else if (dataHex.startsWith("D1C307"))
        {
            detectedKey = 2;
        }
        else if (dataHex.startsWith("D1C287"))
        {
            detectedKey = 3;
        }
        else if (dataHex.startsWith("D1E1CB"))
        {
            detectedKey = 4;
        }
        else if (dataHex.startsWith("D1C397"))
        {
            detectedKey = 4;
        } // Alternate for 4
        else if (dataHex.startsWith("D1C247"))
        {
            detectedKey = 5;
        }
        else if (dataHex.startsWith("D1C357"))
        {
            detectedKey = 6;
        }
        else if (dataHex.startsWith("D1C2D7"))
        {
            detectedKey = 7;
        }
        else if (dataHex.startsWith("D1C3C7"))
        {
            detectedKey = 8;
        }
        else if (dataHex.startsWith("D1C227"))
        {
            detectedKey = 9;
        }
        else if (dataHex.startsWith("D1E017"))
        {
            detectedKey = 10;
        }
        else if (dataHex.startsWith("D1E107"))
        {
            detectedKey = 11;
        }
        else if (dataHex.startsWith("D1E157"))
        {
            detectedKey = 12;
        }

        if (detectedKey >= 0)
        {
            *keyNumber = detectedKey;
            lastKeyPressed = detectedKey;
            lastHexPattern = dataHex;
            return true;
        }
    }

    return false;
}

String KeypadProcessor::getKeyChar(int keyNumber)
{
    // Map Paxton KP75 keypad key numbers to characters
    // Based on actual hardware testing with pattern matching
    // Keypad layout:
    //   1  2  3
    //   4  5  6
    //   7  8  9
    //   *  0  #

    switch (keyNumber)
    {
    case 0:
        return "0";
    case 1:
        return "1";
    case 2:
        return "2";
    case 3:
        return "3";
    case 4:
        return "4";
    case 5:
        return "5";
    case 6:
        return "6";
    case 7:
        return "7";
    case 8:
        return "8";
    case 9:
        return "9";
    case 10:
        return "*";
    case 11:
        return "#";
    case 12:
        return "B";
    default:
        return String(keyNumber);
    }
}
