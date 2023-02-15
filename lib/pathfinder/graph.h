#pragma once
#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <cmath>
#include <limits>
#include "utils/utils.h"

using namespace std;
namespace Pathfinder{
    // 套利路径项
    struct TransactionPathItem {
        string FromToken; // 卖出的token
        double FromPrice = 0;       // 卖出价格
        double FromQuantity = 0;    // 卖出数量 todo 此边depth最大数量，需调整
        string ToToken;   // 买入的token
        double ToPrice = 0;         // 买入价格
        double ToQuantity = 0;      // 买入数量 todo 此边depth最大数量，需调整
    };

    struct ArbitrageChance
    {
        double Profit = 0; // 利润率
        double Quantity = 0; // 起点可执行token数量
        vector<TransactionPathItem> Path;

        TransactionPathItem& FirstStep() {
            return Path.front();
        }

        vector<string> Format() {
            vector<string> info;
            info.push_back(this->Path.front().FromToken);
            for (const auto &item: this->Path) {
                info.push_back(to_string(item.FromPrice));
                info.push_back(item.ToToken);
            }

            return info;
        };
    };

    struct GetExchangePriceReq
    {
        string FromToken = "";
        string ToToken = "";
    };

    struct GetExchangePriceResp
    {
        double FromPrice = 0;
        double ToPrice = 0;
    };

    // 定义一个边的结构体，用于表示两个节点之间的边
    struct Edge {
        int from = 0;   // 起点
        int to = 0;     // 终点
        double weight = 0;  // 权重
        double quantity = 0;  // 数量 todo 现在只存了第一档
        Edge(int f, int t, double w, double q) : from(f), to(t), weight(w), quantity(q) {}
    };

    // 定义一个图的类
    class Graph {
    public:
        Graph();
        ~Graph();

        void AddEdge(const string& from, const string& to, double weight, double quantity);
        int GetExchangePrice(GetExchangePriceReq &req, GetExchangePriceResp &resp); // 路径修正

        ArbitrageChance FindBestPath(string start, string end);
        ArbitrageChance CalculateArbitrage();

    private:
        map<string, int> tokenToIndex;
        map<int, string> indexToToken;

        double fee = 0.0003; // 手续费
        vector<vector<Edge>> nodes; // 存储图中所有的节点及其邻接表
        TransactionPathItem formatTransactionPathItem(int from, int to, double price, double quantity);
        pair<double, vector<TransactionPathItem>> bestOneStep(int start, int end);
        pair<double, vector<TransactionPathItem>> bestTwoStep(int start, int end);
    };
}
