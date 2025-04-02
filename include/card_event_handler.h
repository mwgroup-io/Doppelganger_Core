#ifndef CARD_EVENT_HANDLER_H
#define CARD_EVENT_HANDLER_H

#include "card_processor.h"
#include "email_manager.h"
#include "logger.h"
#include "reset_card_manager.h"
#include "notification_manager.h"

class CardEventHandler
{
public:
    static CardEventHandler &getInstance();
    void begin();
    void handleCardEvent(CardProcessor &cardProcessor);
    void handlePinEvent(CardProcessor &cardProcessor);

private:
    CardEventHandler() = default;
    ~CardEventHandler() = default;
    CardEventHandler(const CardEventHandler &) = delete;
    CardEventHandler &operator=(const CardEventHandler &) = delete;

    void processCardData(CardProcessor &cardProcessor);
    void processPinData(CardProcessor &cardProcessor);
    String formatPinCode(CardProcessor &cardProcessor);
};

#endif