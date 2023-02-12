#pragma once
#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <cmath>
#include <limits>

using namespace std;
namespace Pathfinder{
    // 定义一个无穷大常量
    const double INF = numeric_limits<double>::max();

    // 套利路径项
    struct TransactionPathItem
    {
        std::string FromToken; // 卖出的token
        double FromPrice;	   // 卖出价格
        double FromQuantity;   // 卖出数量
        std::string ToToken;   // 买入的token
        double ToPrice;		   // 买入价格
        double ToQuantity;	   // 买入数量
    };

    struct ArbitrageChance
    {
        double Profit;
        vector<TransactionPathItem> Path;
    };

    struct GetExchangePriceReq
    {
        string FromToken;
        string ToToken;
    };

    struct GetExchangePriceResp
    {
        double FromPrice;
        double ToPrice;
    };

    // 定义一个边的结构体，用于表示两个节点之间的边
    struct Edge {
        int from;   // 起点
        int to;     // 终点
        double weight;  // 权重
        Edge(int f, int t, double w) : from(f), to(t), weight(w) {}
    };

    // 定义一个图的类
    class Graph {
    public:
        Graph();
        ~Graph();

        void AddEdge(string from, string to, double weight);
        int GetExchangePrice(GetExchangePriceReq &req, GetExchangePriceResp &resp); // 路径修正

        string FindBestPath(string start, string end);
        vector<TransactionPathItem> CalculateArbitrage();

    private:
        map<string, int> tokenToIndex;
        map<int, string> indexToToken;

        double fee = 0.0003; // 手续费
        vector<vector<Edge>> nodes; // 存储图中所有的节点及其邻接表
        TransactionPathItem formatTransactionPathItem(int from, int to, double price);
    };
}
