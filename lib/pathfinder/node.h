#pragma once
#include "conf/strategy.h"
#include "utils/utils.h"
#include "lib/api/http/binance_api_wrapper.h"
#include "lib/api/ws/binance_depth_wrapper.h"

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
        Node(int baseIndex, int quoteIndexe2);

        ~Node() = default;

        double GetOriginPrice(int fromIndex, int toIndex);

        double GetParsePrice(int fromIndex, int toIndex);

        double GetParsePathPrice(double price, int fromIndex, int toIndex);

        double GetQuantity(int fromIndex, int toIndex);

        double UpdateSell(vector<WebsocketWrapper::DepthItem> depth, time_t updateTime);

        double UpdateBuy(vector<WebsocketWrapper::DepthItem> depth, time_t updateTime);

        void mockSetOriginPrice(int fromIndex, int toIndex, double price);

        void setBase2Dollar(Node* node);

        void setQuote2Dollar(Node* node);

        int getBaseIndex();

        int getQuoteIndex();

        vector<WebsocketWrapper::DepthItem> getDepth(int index);

        vector<int> mockGetIndexs();

        TransactionPathItem Format(conf::Step& step, map<int, string>& indexToToken, int from, int to);

        time_t updateTime = 0;

    private:
        int baseIndex = 0;   // baseToken序号
        int quoteIndex = 0;     // quoteToken序号
        Node* base2Dollar;
        Node* quote2Dollar;

        vector<WebsocketWrapper::DepthItem> sellDepth;

        vector<WebsocketWrapper::DepthItem> buyDepth;
    };
}