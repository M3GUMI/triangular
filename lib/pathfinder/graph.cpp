#include "graph.h"

namespace Pathfinder{
    Graph::Graph() = default;
    Graph::~Graph() = default;

    void Graph::AddEdge(const string& from, const string& to, double weight, double quantity) {
        // 将边加入相应的节点的邻接表中
        if (not tokenToIndex.count(to)) {
            int index = nodes.size();
            tokenToIndex[to] = index;
            indexToToken[index] = to;

            auto edge = new vector<Edge>;
            nodes.push_back(*edge);
        }

        if (not tokenToIndex.count(from)) {
            int index = nodes.size();
            tokenToIndex[from] = index;
            indexToToken[index] = from;

            auto edge = new vector<Edge>;
            nodes.push_back(*edge);
        }

        int fromIndex = tokenToIndex[from];
        int toIndex = tokenToIndex[to];

        // 边已存在,更新
        for (auto edge: nodes[fromIndex]) {
            if (edge.from == fromIndex && edge.to == toIndex) {
                edge.weight = weight;
                edge.quantity = quantity;
                return;
            }
        }

        // 边不存在, 添加
        auto edge = new Edge(fromIndex, toIndex, weight, quantity);
        nodes[fromIndex].push_back(*edge);
    }

    int Graph::GetExchangePrice(GetExchangePriceReq &req, GetExchangePriceResp &resp)
    {
        // 获取负权图中价格
        resp.FromPrice = 0;
        resp.ToPrice = 0;

        if (not tokenToIndex.count(req.FromToken) || not tokenToIndex.count(req.ToToken)) {
            return define::ErrorGraphNotReady;
        }

        int fromIndex = tokenToIndex[req.FromToken];
        int toIndex = tokenToIndex[req.ToToken];

        for(auto edge:nodes[fromIndex]) {
            if (edge.from==fromIndex && edge.to == toIndex) {
                resp.FromPrice = edge.weight;
            }
        }

        for(auto edge:nodes[fromIndex]) {
            if (edge.from==fromIndex && edge.to == toIndex) {
                resp.ToPrice = edge.weight;
            }
        }

        return 0;
    }

    TransactionPathItem Graph::formatTransactionPathItem(int from, int to, double price, double quantity) {
        TransactionPathItem item{};
        item.FromToken = indexToToken[from];
        item.FromPrice = price;
        item.ToToken = indexToToken[to];
        item.ToPrice = 1 / price; // todo 临时

        return item;
    };

    ArbitrageChance Graph::CalculateArbitrage() {
        vector<TransactionPathItem> path;
        double minRate = 0;

        // 枚举所有三角路径，判断是否存在套利机会。起点为稳定币
        // 穷举起点出发的所有边组合，最终回到起点，是否可套利
        for (const auto &baseToken: conf::BaseAssets) {
            if (not tokenToIndex.count(baseToken.first)) {
                continue;
            };

            int firstToken = tokenToIndex[baseToken.first]; // 起点token
            auto firstEdges = nodes[firstToken]; // 起点出发边

            for (auto &firstEdge: firstEdges) {
                double firstWeight = firstEdge.weight; // 第一条边权重
                int secondToken = firstEdge.to; // 第二token
                auto secondEdges = nodes[secondToken]; // 第二token出发边

                for (auto &secondEdge: secondEdges) {
                    double secondWeight = secondEdge.weight; // 第二条边权重
                    int thirdToken = secondEdge.to; // 第三token
                    auto thirdEdges = nodes[thirdToken]; // 第三token出发边

                    for (auto &thirdEdge: thirdEdges) {
                        double thirdWeight = thirdEdge.weight; // 第三条边权重
                        int endToken = thirdEdge.to; // 第三条边目标token
                        if (firstToken == endToken) {
                            // 对数。乘法处理为加法
                            // 负数。最长路径处理为最短路径
                            double rate = -log(firstWeight) - log(secondWeight) - log(thirdWeight) - 3 * log(1 - fee);
                            if (rate < 0 && rate < minRate) {
                                minRate = rate;
                                path.clear();
                                path.push_back(formatTransactionPathItem(firstToken,secondToken,firstWeight, firstEdge.quantity));
                                path.push_back(formatTransactionPathItem(secondToken,thirdToken,secondWeight, secondEdge.quantity));
                                path.push_back(formatTransactionPathItem(thirdToken,firstToken,thirdWeight, thirdEdge.quantity));
                            }
                        }
                    }
                }
            }
        }

        ArbitrageChance chance{};
        if (path.size() != 3) {
            return chance;
        }

        double profit = 1; // 验算利润率
        vector<string> info;
        for (const auto& item:path) {
            profit = profit*item.FromPrice*(1-0.0003);
        }

        if (profit <= 1) {
            spdlog::error("func: CalculateArbitrage, err: path can not get money");
            return chance;
        }

        chance.Profit = profit;
        chance.Path = path;
        // todo 需要补上数量
        return chance;
    }

    ArbitrageChance Graph::FindBestPath(string start, string end) {
        auto oneStepResult = bestOneStep(tokenToIndex[start], tokenToIndex[end]);
        auto twoStepResult = bestTwoStep(tokenToIndex[start], tokenToIndex[end]);

        ArbitrageChance chance{};
        pair<double, vector<TransactionPathItem>> *data;
        if (twoStepResult.first > oneStepResult.first) {
            data = &twoStepResult;
        } else {
            data = &oneStepResult;
        }

        chance.Profit = data->first;
        chance.Path = data->second;
        // todo 需要补上数量
        return chance;
    }

    // 走一步
    pair<double, vector<TransactionPathItem>> Graph::bestOneStep(int start, int end) {
        vector<TransactionPathItem> path;
        auto edges = nodes[start]; // 起点出发边
        for (auto &edge: edges) {
            if (edge.to != end) {
                continue;
            }
            path.push_back(formatTransactionPathItem(edge.from, edge.to, edge.weight, edge.quantity));
            return make_pair(edge.weight * (1 - fee), path);
        }

        return make_pair(0, path);
    }

    // 走两步
    pair<double, vector<TransactionPathItem>> Graph::bestTwoStep(int start, int end) {
        vector<TransactionPathItem> path;
        double maxRate = 0;

        for (auto &firstEdge: nodes[start]) {
            double firstWeight = firstEdge.weight; // 第一条边权重
            int secondToken = firstEdge.to; // 第二token
            auto secondEdges = nodes[secondToken]; // 第二token出发边

            for (auto &secondEdge: secondEdges) {
                double secondWeight = secondEdge.weight; // 第二条边权重

                if (secondEdge.to == end) {
                    double rate = firstWeight * secondWeight * (1 - fee) * (1 - fee);
                    if (rate > 0 && rate > maxRate) {
                        maxRate = rate;
                        path.clear();
                        path.push_back(
                                formatTransactionPathItem(
                                        firstEdge.from,
                                        firstEdge.to,
                                        firstWeight,
                                        firstEdge.quantity
                                ));
                        path.push_back(
                                formatTransactionPathItem(
                                        secondEdge.from,
                                        secondEdge.to,
                                        secondWeight,
                                        secondEdge.quantity
                                ));
                    }
                }
            }
        }

        return make_pair(maxRate, path);
    }

    /*string Graph::FindBestPath(string fromToken, string toToken) {
        const double INF = numeric_limits<double>::max();
        int start = tokenToIndex[fromToken];
        int end = tokenToIndex[toToken];

        int numNodes = nodes.size(); // 获取节点数
        vector<double> maxProduct(numNodes, 0); // 初始化最大乘积路径
        vector<int> parent(numNodes, -1); // 初始化每个节点的父节点
        vector<int> count(numNodes, 0); // 记录每个节点入队的次数
        vector<bool> inQueue(numNodes, false); // 记录每个节点是否在队列中

        // 将起点的最大乘积路径初始化为1
        maxProduct[start] = 1;

        // 定义一个队列，用于存储待处理的节点
        queue<int> q;
        q.push(start); // 将起点加入队列
        inQueue[start] = true; // 标记起点已经在队列中

        // SPFA算法的主体部分
        while (!q.empty()) {
            int curNode = q.front();
            q.pop();
            inQueue[curNode] = false;

            // 遍历当前节点的邻接表中的所有边
            for (Edge edge : nodes[curNode]) {
                // 计算下一个节点的最大乘积路径
                double newMaxProduct = maxProduct[curNode] * edge.weight;

                // 如果新的最大乘积路径比当前最大乘积路径要大，就更新最大乘积路径，并更新父节点
                if (newMaxProduct > maxProduct[edge.to]) {
                    maxProduct[edge.to] = newMaxProduct;
                    parent[edge.to] = curNode;

                    // 如果下一个节点不在队列中&&没有入过队列，就将其加入队列
                    if (count[edge.to] > 0) {
                        continue;
                    }
                    if (!inQueue[edge.to]) {
                        q.push(edge.to);
                        inQueue[edge.to] = true;
                        count[edge.to]++;

                        // 如果下一个节点入队的次数超过节点数，就说明存在负权回路，需要结束算法
                        if (count[edge.to] >= numNodes) {
                            spdlog::info("func: {}, msg: {}", "FindBestPath", "Negative weight cycle detected!");
                            return "";
                        }
                    }
                }
            }
        }

        // 构造最大乘积路径
        vector<int> path;
        int curNode = end;
        cout << "bestPath: ";
        while (curNode != start) {
            cout << indexToToken[curNode];
            path.push_back(curNode);
            curNode = parent[curNode];
        }
        path.push_back(start);
        cout << indexToToken[start] << endl; // 倒过来看

        int nextStep = path.size() - 2;
        return indexToToken[path[nextStep]];
    }*/
}