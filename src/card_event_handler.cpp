#include "card_event_handler.h"
#include "wifi_setup_manager.h"

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
    if (cardProcessor.getFacilityCode() > 0 && cardProcessor.getCardNumber() > 0)
    {
        processCardData(cardProcessor);
    }
}

void CardEventHandler::handlePinEvent(CardProcessor &cardProcessor)
{
    processPinData(cardProcessor);
}

void CardEventHandler::processCardData(CardProcessor &cardProcessor)
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