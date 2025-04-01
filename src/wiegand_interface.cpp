#include "wiegand_interface.h"
#include "card_processor.h"

// Define global variables
unsigned char databits[MAX_BITS];
volatile unsigned int bitCount = 0;
unsigned char flagDone;
unsigned int weigand_counter;

// Card data storage
volatile unsigned long facilityCode = 0;
volatile unsigned long cardNumber = 0;
volatile unsigned long dataStream = 0;

// HEX conversion buffers
volatile unsigned long bitHolder1 = 0;
volatile unsigned long bitHolder2 = 0;
volatile unsigned long cardChunk1 = 0;
volatile unsigned long cardChunk2 = 0;

// PIV/MF card data buffers
volatile unsigned long pivUID = 0;
volatile unsigned long chunk1Hex = 0;
volatile unsigned long chunk2Hex = 0;
volatile unsigned long paddedChunk2Hex = 0;
volatile unsigned long reverseUID = 0;
String reversedPairsUID = "";
String csvHEX = "";
String dataStreamBIN = "";

// Global instance
WiegandInterface &wiegandInterface = WiegandInterface::getInstance();

WiegandInterface::WiegandInterface() {}

WiegandInterface::~WiegandInterface() {}

WiegandInterface &WiegandInterface::getInstance()
{
    static WiegandInterface instance;
    return instance;
}

void WiegandInterface::begin()
{
    pinMode(DATA0, INPUT);
    pinMode(DATA1, INPUT);

    attachInterrupt(DATA0, handleData0, FALLING);
    attachInterrupt(DATA1, handleData1, FALLING);
    reset();
}

void WiegandInterface::reset()
{
    bitCount = 0;
    flagDone = 0;
    weigand_counter = 0;
    facilityCode = 0;
    cardNumber = 0;
    dataStream = 0;
    bitHolder1 = 0;
    bitHolder2 = 0;
    cardChunk1 = 0;
    cardChunk2 = 0;
    pivUID = 0;
    chunk1Hex = 0;
    chunk2Hex = 0;
    paddedChunk2Hex = 0;
    reverseUID = 0;
    reversedPairsUID = "";
    csvHEX = "";
    dataStreamBIN = "";
}

// Global interrupt handlers
void IRAM_ATTR handleData0()
{
    cardProcessor.ISR_INT0();
}

void IRAM_ATTR handleData1()
{
    cardProcessor.ISR_INT1();
}