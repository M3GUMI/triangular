#pragma once
#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <cmath>
#include <limits>
#include "lib/api/http/binance_api_wrapper.h"
#include "utils/utils.h"

using namespace std;
namespace Pathfinder{
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
        double MinNotional = 0;

        // 预期新币成交额
        double GetNationalQuantity()
        {
            return FormatDoubleV2(Price*Quantity);
        }

        double GetParsePrice()
        {
            if (Side == define::SELL)
            {
                return FormatDoubleV2(Price);
            }
            else
            {
                return FormatDoubleV2(1 / Price);
            }
        }

        double GetParseQuantity()
        {
            if (Side == define::SELL)
            {
                return FormatDoubleV2(Quantity);
            }
            else
            {
                return FormatDoubleV2(Quantity * Price);
            }
        }

        double GetPrice()
        {
            return FormatDoubleV2(Price);
        }

        double GetQuantity()
        {
            return FormatDoubleV2(Quantity);
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

    struct Strategy {
        double Fee;
        define::OrderType OrderType;
        define::TimeInForce TimeInForce;
    };

    struct ArbitrageChance
    {
        double Profit = 0; // 套利利润率。最佳路径计算利润率
        double Quantity = 0; // 起点可执行token数量
        vector<TransactionPathItem> Path;

        TransactionPathItem& FirstStep() {
            return Path.front();
        }

        vector<string> Format() {
            vector<string> info;
            if (empty(this->Path)) {
                return info;
            }
            info.push_back(this->Path.front().GetFromToken());
            for (TransactionPathItem item: this->Path) {
                info.push_back(to_string(item.GetParsePrice()));
                info.push_back(item.GetToToken());
            }

            return info;
        };
    };

    struct GetExchangePriceReq
    {
        string BaseToken = "";
        string QuoteToken = "";
        define::OrderType OrderType;
    };

    struct GetExchangePriceResp
    {
        double SellPrice = 0; // 可卖出的价格
        double SellQuantity = 0; // 可卖出的数量
        double BuyPrice = 0; // 可买入的价格
        double BuyQuantity = 0; // 可买入的数量
    };

    // 定义一个边的结构体，用于表示两个节点之间的边
    struct Edge {
        int from = 0;   // 起点
        int to = 0;     // 终点
        double weight = 0;  //  权重
        double weightQuantity = 0;  // 转换后数量 todo 现在只存了第一档
        double originQuantity = 0;  // 原始数量
        double originPrice = 0;  // 原始价格
        double minNotional = 0;  // 最小成交数量
        bool isSell = true;  // 是否为卖出
    };

    // 定义一个图的类
    class Graph {
    public:
        Graph(HttpWrapper::BinanceApiWrapper &apiWrapper);
        ~Graph();

        void AddEdge(const string& from, const string& to, double originPrice, double quantity, double minNotional, bool isFrom);
        int GetExchangePrice(GetExchangePriceReq &req, GetExchangePriceResp &resp); // 路径修正

        ArbitrageChance FindBestPath(string name, string start, string end, double quantity);
        ArbitrageChance CalculateArbitrage(const string& strategy);

    protected:
        HttpWrapper::BinanceApiWrapper &apiWrapper;
    private:
        map<string, int> tokenToIndex;
        map<int, string> indexToToken;

        vector<vector<Edge>> nodes; // 存储图中所有的节点及其邻接表
        TransactionPathItem formatTransactionPathItem(Edge& edge, Strategy& strategy);
        static void adjustQuantities(vector<TransactionPathItem>& items);
        pair<double, vector<TransactionPathItem>> bestOneStep(int start, int end, Strategy& strategy);
        pair<double, vector<TransactionPathItem>> bestTwoStep(int start, int end, Strategy& strategy);

        bool checkToken(int token);
        double calProfit(Strategy &strategy, vector<TransactionPathItem> &path);
    };
}
