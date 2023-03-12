#pragma once
#include "define/define.h"
#include "utils/utils.h"

struct OrderData {
    uint64_t OrderId = 0;
    int Phase = 0;

    string BaseToken;
    string QuoteToken;
    double Price = 0;
    double Quantity = 0;

    define::OrderSide Side;
    define::OrderType OrderType;
    define::TimeInForce TimeInForce;

    define::OrderStatus OrderStatus; // 订单状态
    double ExecutePrice = 0; // 成交价格 // todo market单的price为空，此处计算错误
    double ExecuteQuantity = 0; // 已成交数量
    double CummulativeQuoteQuantity = 0; // 新token数量

    uint64_t UpdateTime = 0; // 最后一次更新实际

    double GetParsePrice()
    {
        if (Side == define::SELL)
        {
            return RoundDouble(Price);
        }
        else
        {
            return RoundDouble(1 / Price);
        }
    }

    string GetFromToken() {
        if (Side == define::SELL) {
            return BaseToken;
        } else {
            return QuoteToken;
        }
    }

    string GetToToken() {
        if (Side == define::SELL) {
            return QuoteToken;
        } else {
            return BaseToken;
        }
    }

    // 预期新币成交额
    double GetNationalQuantity()
    {
        return RoundDouble(Price*Quantity);
    }

    double GetUnExecuteQuantity()
    {
        if (Side == define::SELL) {
            return RoundDouble(Quantity - ExecuteQuantity);
        } else {
            return RoundDouble(Quantity*ExecutePrice - CummulativeQuoteQuantity);
        }
    }

    double GetExecuteQuantity() {
        if (Side == define::SELL) {
            return RoundDouble(ExecuteQuantity);
        } else {
            return RoundDouble(CummulativeQuoteQuantity);
        }
    }

    // 实际新币数量
    double GetNewQuantity()
    {
        if (Side == define::SELL) {
            return RoundDouble(CummulativeQuoteQuantity);
        } else {
            return RoundDouble(CummulativeQuoteQuantity/ExecutePrice);
        }
    }
};

define::OrderStatus stringToOrderStatus(string status);
define::OrderSide stringToSide(string side);
string sideToString(uint32_t side);
string orderTypeToString(uint32_t orderType);
string timeInForceToString(uint32_t timeInForce);