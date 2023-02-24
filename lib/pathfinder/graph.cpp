#include "graph.h"

namespace Pathfinder{
    Graph::Graph() = default;
    Graph::~Graph() = default;

    void Graph::AddEdge(const string& from, const string& to, double originPrice, double quantity, bool isSell) {
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
        for (auto &edge: nodes[fromIndex]) {
            if (edge.from == fromIndex && edge.to == toIndex) {
                edge.originPrice = originPrice;
                edge.originQuantity = quantity;
                edge.isSell = isSell;
                if (isSell) {
                    edge.weight = -log(originPrice);
                    edge.weightQuantity = quantity;
                } else {
                    edge.weight = log(originPrice);
                    edge.weightQuantity = quantity/originPrice;
                }

                return;
            }
        }

        // 边不存在, 添加
        auto edge = new Edge();
        edge->from = fromIndex;
        edge->to = toIndex;
        edge->originPrice = originPrice;
        edge->originQuantity = quantity;
        edge->isSell = isSell;
        if (isSell) {
            edge->weight = -log(originPrice);
            edge->weightQuantity = quantity;
        } else {
            edge->weight = log(originPrice);
            edge->weightQuantity = quantity/originPrice;
        }

        nodes[fromIndex].push_back(*edge);
    }

    int Graph::GetExchangePrice(GetExchangePriceReq &req, GetExchangePriceResp &resp)
    {
        // 获取负权图中价格
        resp.SellPrice = 0;
        resp.BuyPrice = 0;

        if (not tokenToIndex.count(req.FromToken) || not tokenToIndex.count(req.ToToken)) {
            return define::ErrorGraphNotReady;
        }

        int fromIndex = tokenToIndex[req.FromToken];
        int toIndex = tokenToIndex[req.ToToken];

        // todo edge存储方式优化一下，以下逻辑需要调整
        for(auto edge:nodes[fromIndex]) {
            // 我方可卖出价格
            if (edge.from==fromIndex && edge.to == toIndex) { // 盘口卖出价格，己方
                // spdlog::debug("sell price: {}, fromIndex: {}, toIndex: {}", edge.originPrice, indexToToken[edge.from], indexToToken[edge.to]);
                resp.SellPrice = edge.originPrice;
                resp.SellQuantity = edge.originQuantity;
            }
        }

        for(auto edge:nodes[toIndex]) {
            // 我方可买入价格
            if (edge.to==fromIndex && edge.from == toIndex) { // 盘口卖出价格，己方
                // spdlog::debug("buy price: {}, fromIndex: {}, toIndex: {}", edge.originPrice, indexToToken[edge.to], indexToToken[edge.from]);
                resp.BuyPrice = edge.originPrice;
                resp.BuyQuantity = edge.originQuantity;
            }
        }

        if (resp.SellPrice == 0 || resp.BuyPrice == 0) {
            return define::ErrorGraphNotReady;
        }

        return 0;
    }

    TransactionPathItem Graph::formatTransactionPathItem(Edge& edge) {
        TransactionPathItem item{};
        if (edge.isSell) {
            item.BaseToken = indexToToken[edge.from];
            item.QuoteToken = indexToToken[edge.to];
            item.Side = define::SELL;

            item.Price = edge.originPrice;
            item.Quantity = edge.originPrice;
        } else {
            item.BaseToken = indexToToken[edge.to];
            item.QuoteToken = indexToToken[edge.from];
            item.Side = define::BUY;

            item.Price = edge.originPrice;
            item.Quantity = edge.originPrice;
        }

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
                            double rate = firstWeight + secondWeight + thirdWeight - 3 * log(1 - fee);
                            if (rate < 0 && rate < minRate) {
                                minRate = rate;
                                path.clear();
                                path.push_back(formatTransactionPathItem(firstEdge));
                                path.push_back(formatTransactionPathItem(secondEdge));
                                path.push_back(formatTransactionPathItem(thirdEdge));
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
            if (item.Side == define::SELL) {
                profit = profit*item.Price*(1-this->fee);
            } else {
                profit = profit*(1/item.Price)*(1-this->fee);
            }
        }

        if (profit <= 1) {
            spdlog::error("func: CalculateArbitrage, err: path can not get money");
            return chance;
        }

        adjustQuantities(path);
        chance.Profit = profit;
        chance.Quantity = path.front().Quantity;
        chance.Path = path;
        return chance;
    }

    ArbitrageChance Graph::FindBestPath(string start, string end, double quantity) {
        auto oneStepResult = bestOneStep(tokenToIndex[start], tokenToIndex[end]);
        auto twoStepResult = bestTwoStep(tokenToIndex[start], tokenToIndex[end]);

        ArbitrageChance chance{};
        pair<double, vector<TransactionPathItem>> *data;
        if (twoStepResult.first < oneStepResult.first) {
            data = &twoStepResult;
        } else {
            data = &oneStepResult;
        }

        auto pathPrice = data->second.front().Price;
        auto pathQuantity = data->second.front().Quantity;
        if (data->second.front().Side == define::SELL) {
            data->second.front().Quantity = quantity <= pathQuantity? quantity: pathQuantity;
        } else {
            quantity /= pathPrice; // 转换成baseToken数量
            data->second.front().Quantity = quantity <= pathQuantity? quantity: pathQuantity;
        }

        adjustQuantities(data->second);
        chance.Profit = data->first;
        chance.Quantity = data->second.front().Quantity;
        chance.Path = data->second;
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
            path.push_back(formatTransactionPathItem(edge));
            return make_pair(edge.weight + log(1 - fee), path);
        }

        return make_pair(0, path);
    }

    // 走两步
    pair<double, vector<TransactionPathItem>> Graph::bestTwoStep(int start, int end) {
        vector<TransactionPathItem> path;
        double minRate = 0;

        for (auto &firstEdge: nodes[start]) {
            double firstWeight = firstEdge.weight; // 第一条边权重
            int secondToken = firstEdge.to; // 第二token
            auto secondEdges = nodes[secondToken]; // 第二token出发边

            for (auto &secondEdge: secondEdges) {
                double secondWeight = secondEdge.weight; // 第二条边权重

                if (secondEdge.to == end) {
                    double rate = firstWeight + secondWeight - 2 * log(1 - fee);
                    if (rate < 0 && rate < minRate) {
                        minRate = rate;
                        path.clear();
                        path.push_back(formatTransactionPathItem(firstEdge));
                        path.push_back(formatTransactionPathItem(secondEdge));
                    }
                }
            }
        }

        return make_pair(minRate, path);
    }

    void Graph::adjustQuantities(vector<TransactionPathItem>& items) {
        double cumQuantity = items[0].Quantity;
        // spdlog::info("token: {}, quantity: {}", items[0].FromToken, cumQuantity);
        for (int i = 0; i < items.size(); i++) {
            // 获取第i项以前的累加数量
            auto &curItem = items[i];
            // 判断第i项容量
            if (cumQuantity <= curItem.Quantity) {
                // 容量足够，直接赋值
                curItem.Quantity = cumQuantity;
                if (curItem.Side == define::SELL) {
                    cumQuantity = curItem.Quantity*curItem.Price;
                } else {
                    cumQuantity = curItem.Quantity/curItem.Price;
                }
                // spdlog::info("i: {}, token: {}, over, newQuantity: {}", i, curItem.FromToken+curItem.ToToken, curItem.FromQuantity);
            } else {
                // 容量不足，逆推数量
                // spdlog::info("i: {}, token: {}, less, need: {}, have: {}", i, curItem.FromToken+curItem.ToToken, cumQuantity, curItem.FromQuantity);
                cumQuantity = curItem.Quantity;
                double reverseCumQuantity = curItem.Quantity;
                for (int k = i - 1; k >= 0; k--) {
                    if (items[k].Side == define::SELL) {
                        reverseCumQuantity /= items[k].Price;
                        items[k].Quantity = reverseCumQuantity;
                    } else {
                        reverseCumQuantity *= items[k].Price;
                        items[k].Quantity = reverseCumQuantity;
                    }
                    // spdlog::info("k: {}, token: {}, newQuantity: {}", k, items[k].FromToken+items[k].ToToken, items[k].FromQuantity);
                }
            }
        }
    }
}