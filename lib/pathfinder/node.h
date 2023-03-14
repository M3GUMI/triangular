#pragma once
#include "conf/strategy.h"
#include "utils/utils.h"

namespace Pathfinder
{
    // 套利路径项
    struct TransactionPathItem
    {
        string BaseToken;
        string QuoteToken;
        define::OrderSide Side;
        define::OrderType OrderType;
        define::TimeInForce TimeInForce;
        double Price = 0;
        double Quantity = 0;

        // 预期新币成交额
        double GetNationalQuantity()
        {
            return RoundDouble(Price*Quantity);
        }

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

        double GetParseQuantity()
        {
            if (Side == define::SELL)
            {
                return RoundDouble(Quantity);
            }
            else
            {
                return RoundDouble(Quantity * Price);
            }
        }

        double GetPrice()
        {
            return RoundDouble(Price);
        }

        double GetQuantity()
        {
            return RoundDouble(Quantity);
        }

        string GetFromToken()
        {
            if (Side == define::SELL)
            {
                return BaseToken;
            }
            else
            {
                return QuoteToken;
            }
        }

        string GetToToken()
        {
            if (Side == define::SELL)
            {
                return QuoteToken;
            }
            else
            {
                return BaseToken;
            }
        }
    };

    // 交易节点
    class Node
    {
    public:
        Node(int baseIndex, int quoteIndex);

        ~Node() = default;

        double GetOriginPrice(int fromIndex, int toIndex);

        double GetParsePrice(int fromIndex, int toIndex);

        double GetQuantity(int fromIndex, int toIndex);

        double UpdateSell(double price, double quantity);

        double UpdateBuy(double price, double quantity);

        void mockSetOriginPrice(int fromIndex, int toIndex, double price);

        vector<int> mockGetIndexs();

        TransactionPathItem Format(conf::Step& step, map<int, string>& indexToToken, int from, int to);

    private:
        int baseIndex = 0;   // baseToken序号
        int quoteIndex = 0;     // quoteToken序号

        double sellPrice = 0;  // baseToken卖出, 买入quoteToken的原始价格
        double sellQuantity = 0;  // 卖出原始数量

        double buyPrice = 0;  // baseToken买入, 卖出quoteToken的原始价格
        double buyQuantity = 0;  // 买入原始数量
    };
}