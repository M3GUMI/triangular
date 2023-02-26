#pragma once
#include "define/define.h"
#include "utils/utils.h"

struct OrderData {
    uint64_t OrderId = 0;
    string BaseToken;
    string QuoteToken;
    double Price = 0;
    double Quantity = 0;

    define::OrderSide Side;
    define::OrderType OrderType;
    define::TimeInForce TimeInForce;

    define::OrderStatus OrderStatus; // 订单状态
    double ExecuteQuantity = 0; // 已成交数量，经过一层sell、buy转换
    double CummulativeQuoteQuantity = 0; // 新token数量

    uint64_t UpdateTime = 0; // 最后一次更新实际

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


    double GetUnExecuteQuantity()
    {
        if (Side == define::SELL) {
            return FormatDoubleV2(Quantity - ExecuteQuantity);
        } else {
            return FormatDoubleV2(Quantity*Price - CummulativeQuoteQuantity);
        }
    }

    double GetExecuteQuantity() {
        if (Side == define::SELL) {
            return FormatDoubleV2(ExecuteQuantity);
        } else {
            return FormatDoubleV2(CummulativeQuoteQuantity);
        }
    }

    double GetNewQuantity()
    {
        if (Side == define::SELL) {
            return FormatDoubleV2(CummulativeQuoteQuantity);
        } else {
            return FormatDoubleV2(CummulativeQuoteQuantity/Price);
        }
    }
};

define::OrderStatus stringToOrderStatus(string status);
define::OrderSide stringToSide(string side);
string sideToString(uint32_t side);
string orderTypeToString(uint32_t orderType);
string timeInForceToString(uint32_t timeInForce);