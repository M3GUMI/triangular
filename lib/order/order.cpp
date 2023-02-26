#include "order.h"

define::OrderStatus stringToOrderStatus(string status)
{
    if (status == "NEW")
    {
        return define::NEW;
    }
    else if (status == "PARTIALLY_FILLED")
    {
        return define::PARTIALLY_FILLED;
    }
    else if (status == "FILLED")
    {
        return define::FILLED;
    }
    else if (status == "CANCELED")
    {
        return define::CANCELED;
    }
    else if (status == "PENDING_CANCEL")
    {

        return define::PENDING_CANCEL;
    }
    else if (status == "REJECTED")
    {
        return define::REJECTED;
    }
    else if (status == "EXPIRED")
    {

        return define::EXPIRED;
    }

    return define::INVALID_ORDER_STATUS;
}

define::OrderSide stringToSide(string side)
{
    if (side == "BUY") {
        return define::BUY;
    } else if (side == "SELL") {
        return define::SELL;
    }

    return define::INVALID_SIDE;
}

string sideToString(uint32_t side)
{
    switch (side)
    {
        case define::BUY:
            return "BUY";
        case define::SELL:
            return "SELL";
    }

    return "";
}

string orderTypeToString(uint32_t orderType)
{
    switch (orderType)
    {
        case define::LIMIT:
            return "LIMIT";
        case define::MARKET:
            return "MARKET";
        case define::STOP_LOSS:
            return "STOP_LOSS";
        case define::STOP_LOSS_LIMIT:
            return "STOP_LOSS_LIMIT";
        case define::TAKE_PROFIT:
            return "TAKE_PROFIT";
        case define::TAKE_PROFIT_LIMIT:
            return "TAKE_PROFIT_LIMIT";
        case define::LIMIT_MAKER:
            return "LIMIT_MAKER";
    }

    return "";
}

string timeInForceToString(uint32_t timeInForce)
{
    switch (timeInForce)
    {
        case define::GTC:
            return "GTC";
        case define::IOC:
            return "IOC";
        case define::FOK:
            return "FOK";
    }

    return "";
}
