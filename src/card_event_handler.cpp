#include "card_event_handler.h"
#include "wifi_setup_manager.h"
#include "keypad_processor.h"

extern ReaderManager &readerManager;

CardEventHandler &CardEventHandler::getInstance()
{
    static CardEventHandler instance;
    return instance;
}

void CardEventHandler::begin()
{
    // Any initialization if needed
}

void CardEventHandler::handleCardEvent(CardProcessor &cardProcessor)
{
    // Log card data to CSV based on current reader type
    ReaderType currentMode = readerManager.getCurrentType();

    if (currentMode == READER_HID)
    {
        processHIDCardData(cardProcessor);
    }
    else if (currentMode == READER_PAXTON)
    {
        processNet2CardData(cardProcessor);
    }
}

void CardEventHandler::handlePinEvent(CardProcessor &cardProcessor)
{
    processPinData(cardProcessor);
}

void CardEventHandler::handleKeypadEvent(CardProcessor &cardProcessor)
{
    processKeypadData(cardProcessor);
}

void CardEventHandler::processHIDCardData(CardProcessor &cardProcessor)
{
    logger.writeCardLog();

    // Send notification (which handles email if configured)
    String cardData = String(cardProcessor.getBitCount()) + "," +
                      String(cardProcessor.getFacilityCode()) + "," +
                      String(cardProcessor.getCardNumber()) + "," +
                      String(cardProcessor.getCsvHEX()) + "," +
                      String(cardProcessor.getDataStreamBIN());
    notificationManager.handleCardRead(cardData.c_str());
}

void CardEventHandler::processNet2CardData(CardProcessor &cardProcessor)
{
    logger.writeCardLog();

    // Send notification (which handles email if configured)
    String cardData = String(cardProcessor.getBitCount()) + "," +
                      String(cardProcessor.getFacilityCode()) + "," +
                      String(cardProcessor.getCardNumber()) + "," +
                      String(cardProcessor.getCsvHEX()) + "," +
                      String(cardProcessor.getDataStreamBIN());
    notificationManager.handleCardRead(cardData.c_str());
}

void CardEventHandler::processPinData(CardProcessor &cardProcessor)
{
    logger.writePinLog();

    // Send notification (which handles email if configured)
    String pinCode = formatPinCode(cardProcessor);
    String pinData = pinCode + "," + String(cardProcessor.getDataStreamBIN());
    notificationManager.handlePinRead(pinData.c_str());
}

void CardEventHandler::processKeypadData(CardProcessor &cardProcessor)
{
    logger.writeKeypadLog();

    // Send notification (which handles email if configured)
    int keyNum = cardProcessor.getKeypadNumber();
    String keyChar = keypadProcessor.getKeyChar(keyNum);
    String keypadData = keyChar + "," + String(cardProcessor.getDataStreamBIN());
    notificationManager.handlePinRead(keypadData.c_str());
}

String CardEventHandler::formatPinCode(CardProcessor &cardProcessor)
{
    if ((cardProcessor.getBitHolder1() == 10) && cardProcessor.getBitCount() == 4)
    {
        return "*";
    }
    else if ((cardProcessor.getBitHolder1() == 11) && cardProcessor.getBitCount() == 4)
    {
        return "#";
    }
    else if ((cardProcessor.getBitHolder1() >= 0 && cardProcessor.getBitHolder1() <= 9) &&
             cardProcessor.getBitCount() == 4)
    {
        return String(cardProcessor.getBitHolder1());
    }
    else
    {
        return "ERROR";
    }
}

// Global instance
CardEventHandler &cardEventHandler = CardEventHandler::getInstance();