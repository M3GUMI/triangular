#pragma once
#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <cmath>
#include <limits>
#include "lib/api/http/binance_api_wrapper.h"
#include "lib/api/ws/binance_depth_wrapper.h"
#include "node.h"

using namespace std;
namespace Pathfinder{
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
        string BaseToken;
        string QuoteToken;
        define::OrderType OrderType;
    };

    struct GetExchangePriceResp
    {
        double SellPrice = 0; // 可卖出的价格
        double SellQuantity = 0; // 可卖出的数量
        double BuyPrice = 0; // 可买入的价格
        double BuyQuantity = 0; // 可买入的数量
    };

    // 定义一个图的类
    class Graph {
    public:
        Graph(HttpWrapper::BinanceApiWrapper &apiWrapper);
        ~Graph();

        int GetExchangePrice(GetExchangePriceReq &req, GetExchangePriceResp &resp); // 路径修正
        ArbitrageChance CalculateArbitrage(const string& strategy);
        ArbitrageChance FindBestPath(const string& name, const string& origin, const string& end, double quantity);

    protected:
        HttpWrapper::BinanceApiWrapper &apiWrapper;

        void Init(vector<HttpWrapper::BinanceSymbolData> &data);

        void UpdateNode(WebsocketWrapper::DepthData& data);

    private:
        // 套利计算分组，一次只算500个环。取值为0-499、500-999、1000-1499、1500-1999、2000-2113
        int indexStart = 0;
        int groupSize = 500;

        map<string, int> tokenToIndex{};
        map<int, string> indexToToken{};

        map<u_int64_t, Node*> tradeNodeMap{}; // key为baseIndex+quoteIndex、quoteIndex+baseIndex两种类型
        map<int, vector<vector<int>>> triangularMap{}; // 存储所有key起点的三元环
        map<u_int64_t, vector<vector<int>>> bestPathMap{}; // 存储所有两点间路径，最长两步。key前32位代表起点，后32位代表终点

        vector<TransactionPathItem> formatPath(Strategy& strategy, vector<int>& path);
        double calculateProfit(Strategy& strategy, vector<int>& path);
        bool checkPath(vector<TransactionPathItem>& path);
        static u_int64_t formatKey(int from, int to);
        static void adjustQuantities(vector<TransactionPathItem>& items);
    };
}
