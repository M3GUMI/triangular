#pragma once
#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <cmath>
#include <limits>
#include <functional>
#include "lib/api/http/binance_api_wrapper.h"
#include "lib/api/ws/binance_depth_wrapper.h"
#include "conf/strategy.h"
#include <unordered_map>
#include "node.h"
#include <set>
#include <string>
#include <list>

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

    struct FindBestPathReq
    {
        conf::Strategy Strategy; // 策略
        string Origin; // 起点
        string End; // 终点
        double Quantity; // 数量
        int Phase = 0; // 阶段
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
        time_t UpdateTime = 0; // 更新时间
    };

    struct BestPath
    {
        vector<int> bestPath;
        double profit;
    };

    // 定义一个图的类
    class Graph {
    public:
        function<void(ArbitrageChance& chance)> subscriber = nullptr;
        function<void(const string& base, string quote, double buyPrice, double sellPrice)> mockSubscriber = nullptr;
        Graph(HttpWrapper::BinanceApiWrapper &apiWrapper);
        ~Graph();

        int GetExchangePrice(GetExchangePriceReq &req, GetExchangePriceResp &resp); // 路径修正
        ArbitrageChance CalculateArbitrage(conf::Strategy& strategy, int baseIndex, int quoteIndex);
        ArbitrageChance FindBestPath(FindBestPathReq& req);
        void SubscribeArbitrage(function<void(ArbitrageChance& chance)> handler);      // 订阅套利机会推送
        void SubscribeMock(function<void(const string& base, string quote, double buyPrice, double sellPrice)> handler);
        double toDollar(int fromIndex); // 获得币到美元的汇率
        map<string, int> tokenToIndex{};
        map<int, string> indexToToken{};

    protected:
        HttpWrapper::BinanceApiWrapper &apiWrapper;

        void Init(vector<HttpWrapper::BinanceSymbolData> &data);

        void UpdateNode(WebsocketWrapper::DepthData& data);

    private:
        // 套利计算分组，一次只算500个环。取值为0-499、500-999、1000-1499、1500-1999、2000-2113
        int indexStart = 0;
        int groupSize = 500;

        struct Triangular
        {
            vector<int> Steps;
        };

        struct Path
        {
            int StepCount = 0; // 步数
            vector<int> Steps;
        };

        map<u_int64_t, Node*> tradeNodeMap{}; // key为baseIndex+quoteIndex、quoteIndex+baseIndex两种类型
        map<int, vector<Triangular*>> triangularMap{}; // 存储所有key起点的三元环
        map<u_int64_t, vector<Path*>> pathMap{}; // 存储所有两点间路径，最长两步。key前32位代表起点，后32位代表终点

        map<u_int64_t, vector<Triangular*>> relatedTriangular{}; // 存储交易对对应的三元环
        map<u_int64_t, set<Path*>> relatedPath{}; // 交易对印象路径的倒排索引
        map<u_int64_t, list<BestPath>> bestPathMap{};

        vector<TransactionPathItem> formatPath(conf::Strategy& strategy, int phase, vector<int>& path);
        double calculateProfit(conf::Strategy& strategy, int phase, vector<int>& path);
        bool checkPath(vector<TransactionPathItem>& path);
        int updateBestMap(int from, int to);
        double calculateMakerPathProfit(vector<int>& path);
        static u_int64_t formatKey(int from, int to);
        static void adjustQuantities(vector<TransactionPathItem>& items);
        double GetPathPrice(int fromIndex, int toIndex);
        double ToDollar(int fromIndex);
    };
}
