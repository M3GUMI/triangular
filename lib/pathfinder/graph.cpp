#include <cfloat>
#include "graph.h"

namespace Pathfinder{
    Graph::Graph(HttpWrapper::BinanceApiWrapper &apiWrapper): apiWrapper(apiWrapper) {
    }
    Graph::~Graph() = default;

    void Graph::AddEdge(const string& from, const string& to, double originPrice, double quantity, double minNotional, bool isSell) {
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
                edge.minNotional = minNotional;
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
        edge->minNotional = minNotional;
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

        if (not tokenToIndex.count(req.BaseToken) || not tokenToIndex.count(req.QuoteToken)) {
            return define::ErrorGraphNotReady;
        }

        auto symbolData = apiWrapper.GetSymbolData(req.BaseToken, req.QuoteToken);
        int baseIndex = tokenToIndex[req.BaseToken];
        int quoteIndex = tokenToIndex[req.QuoteToken];

        // todo edge存储方式优化一下，以下逻辑需要调整
        for(auto edge:nodes[baseIndex]) {
            // 我方可卖出价格
            if (edge.from==baseIndex && edge.to == quoteIndex) { // 盘口卖出价格，己方
                resp.SellPrice = edge.originPrice;
                resp.SellQuantity = edge.originQuantity;

                // maker不可下单到对手盘
                if (req.OrderType == define::LIMIT_MAKER) {
                    resp.SellPrice = resp.SellPrice + symbolData.TicketSize;
                }
            }
        }

        for(auto edge:nodes[quoteIndex]) {
            // 我方可买入价格
            if (edge.to==baseIndex && edge.from == quoteIndex) { // 盘口卖出价格，己方
                resp.BuyPrice = edge.originPrice;
                resp.BuyQuantity = edge.originQuantity;
                // maker不可下单到对手盘
                if (req.OrderType == define::LIMIT_MAKER) {
                    resp.BuyPrice = resp.BuyPrice - symbolData.TicketSize;
                }
            }
        }

        if (resp.SellPrice == 0 || resp.BuyPrice == 0) {
            return define::ErrorGraphNotReady;
        }

        return 0;
    }

    TransactionPathItem Graph::formatTransactionPathItem(Edge& edge, Strategy& strategy) {
        TransactionPathItem item{};
        if (edge.isSell) {
            item.BaseToken = indexToToken[edge.from];
            item.QuoteToken = indexToToken[edge.to];
            item.Side = define::SELL;
        } else {
            item.BaseToken = indexToToken[edge.to];
            item.QuoteToken = indexToToken[edge.from];
            item.Side = define::BUY;
        }

        item.OrderType = strategy.OrderType;
        item.TimeInForce = strategy.TimeInForce;
        item.Price = edge.originPrice;
        item.Quantity = edge.originQuantity;
        item.MinNotional = edge.minNotional;

        return item;
    };

    ArbitrageChance Graph::CalculateArbitrage(const string& name) {
        Strategy strategy{};
        if (name == "maker") {
            strategy.Fee = 0;
            strategy.OrderType = define::LIMIT_MAKER;
            strategy.TimeInForce = define::GTC;
        } else {
            strategy.Fee = 0.0004;
            strategy.OrderType = define::LIMIT;
            strategy.TimeInForce = define::IOC;
        }

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
                if (checkToken(secondToken)) {
                    continue;
                }

                for (auto &secondEdge: secondEdges) {
                    double secondWeight = secondEdge.weight; // 第二条边权重
                    int thirdToken = secondEdge.to; // 第三token
                    auto thirdEdges = nodes[thirdToken]; // 第三token出发边
                    if (checkToken(thirdToken)) {
                        continue;
                    }

                    for (auto &thirdEdge: thirdEdges) {
                        double thirdWeight = thirdEdge.weight; // 第三条边权重
                        int endToken = thirdEdge.to; // 第三条边目标token
                        if (firstToken == endToken) {
                            // 对数。乘法处理为加法
                            // 负数。最长路径处理为最短路径
                            double rate = firstWeight + secondWeight + thirdWeight - 3 * log(1 - strategy.Fee);
                            if (rate < 0 && rate < minRate) {
                                vector<TransactionPathItem> newPath;
                                newPath.clear();
                                newPath.push_back(formatTransactionPathItem(firstEdge, strategy));
                                newPath.push_back(formatTransactionPathItem(secondEdge, strategy));
                                newPath.push_back(formatTransactionPathItem(thirdEdge, strategy));
                                adjustQuantities(newPath);

                                bool lessMinNotional;
                                for (TransactionPathItem &item:newPath) {
                                    // 检查最小成交金额
                                    if (item.MinNotional > item.GetNationalQuantity()) {
                                        lessMinNotional = true;
                                    }
                                }

                                if (lessMinNotional) {
                                    continue;
                                }

                                if (newPath.front().Quantity >= conf::MinTriangularQuantity) {
                                    minRate = rate;
                                    path = newPath;
                                }
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

        double profit = calProfit(strategy, path);
        if (profit <= 1) {
            spdlog::error("func: CalculateArbitrage, err: path can not get money");
            return chance;
        }

        chance.Profit = profit;
        chance.Quantity = path.front().Quantity;
        chance.Path = path;
        return chance;
    }

    ArbitrageChance Graph::FindBestPath(FindBestPathReq& req) {
        Strategy strategy{};
        if (req.Name == "maker") {
            strategy.Fee = 0;
            strategy.OrderType = define::LIMIT_MAKER;
            strategy.TimeInForce = define::GTC;
        } else {
            strategy.Fee = 0.0004;
            strategy.OrderType = define::LIMIT;
            strategy.TimeInForce = define::IOC;
        }

        auto oneStepResult = bestOneStep(tokenToIndex[req.Origin], tokenToIndex[req.End], strategy);
        auto twoStepResult = bestTwoStep(tokenToIndex[req.Origin], tokenToIndex[req.End], strategy);

        ArbitrageChance chance{};
        pair<double, vector<TransactionPathItem>> *data;
        if (twoStepResult.first < oneStepResult.first) {
            data = &twoStepResult;
        } else {
            data = &oneStepResult;
        }

        auto pathPrice = data->second.front().Price;
        auto pathQuantity = data->second.front().Quantity;
        auto quantity = req.Quantity;
        double realQuantity = 0;
        if (data->second.front().Side == define::SELL) {
            spdlog::info("quantity: {}, pathQuantity: {}", quantity, pathQuantity);
            realQuantity = quantity <= pathQuantity? quantity: pathQuantity;
        } else {
            quantity /= pathPrice; // 转换成baseToken数量
            spdlog::info("quantity: {}, pathQuantity: {}", quantity, pathQuantity);
            realQuantity = quantity <= pathQuantity? quantity: pathQuantity;
        }

        data->second.front().Quantity = realQuantity;
        chance.Profit = calProfit(strategy, data->second);
        chance.Quantity = RoundDouble(realQuantity);
        chance.Path = data->second;
        return chance;
    }

    // 走一步
    pair<double, vector<TransactionPathItem>> Graph::bestOneStep(int start, int end, Strategy& strategy) {
        vector<TransactionPathItem> path;
        auto edges = nodes[start]; // 起点出发边
        for (auto &edge: edges) {
            if (edge.to != end) {
                continue;
            }
            path.push_back(formatTransactionPathItem(edge, strategy));
            adjustQuantities(path);
            return make_pair(edge.weight + log(1 - strategy.Fee), path);
        }

        return make_pair(DBL_MAX, path);
    }

    // 走两步
    pair<double, vector<TransactionPathItem>> Graph::bestTwoStep(int start, int end, Strategy& strategy) {
        vector<TransactionPathItem> path;
        double minRate = DBL_MAX;

        for (auto &firstEdge: nodes[start]) {
            double firstWeight = firstEdge.weight; // 第一条边权重
            int secondToken = firstEdge.to; // 第二token
            auto secondEdges = nodes[secondToken]; // 第二token出发边
            if (checkToken(secondToken)) {
                continue;
            }

            for (auto &secondEdge: secondEdges) {
                double secondWeight = secondEdge.weight; // 第二条边权重

                if (secondEdge.to == end) {
                    double rate = firstWeight + secondWeight - 2 * log(1 - strategy.Fee);
                    if (rate < minRate) {
                        vector<TransactionPathItem> newPath;
                        newPath.clear();
                        newPath.push_back(formatTransactionPathItem(firstEdge, strategy));
                        newPath.push_back(formatTransactionPathItem(secondEdge, strategy));
                        adjustQuantities(newPath);

                        bool lessMinNotional;
                        for (TransactionPathItem &item:newPath)
                        {
                            // 检查最小成交金额
                            if (item.MinNotional > item.GetNationalQuantity())
                            {
                                lessMinNotional = true;
                            }
                        }

                        if (lessMinNotional) {
                            continue;
                        }

                        minRate = rate;
                        path = newPath;
                    }
                }
            }
        }

        return make_pair(minRate, path);
    }

    void Graph::adjustQuantities(vector<TransactionPathItem>& items) {
        double cumQuantity = items[0].Quantity;
        // spdlog::info("token: {}, quantity: {}", items[0].Quantity, cumQuantity);
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
                // spdlog::info("i: {}, token: {}, over, newQuantity: {}", i, curItem.GetFromToken()+curItem.GetToToken(), curItem.Quantity);
            } else {
                // 容量不足，逆推数量
                // spdlog::info("i: {}, token: {}, less, need: {}, have: {}", i, curItem.GetFromToken()+curItem.GetToToken(), cumQuantity, curItem.Quantity);
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
                    // spdlog::info("k: {}, token: {}, newQuantity: {}", k, items[k].GetFromToken()+items[k].GetToToken(), items[k].Quantity);
                }
            }
        }
    }

    // 摘掉usdt
    bool Graph::checkToken(int token) {
        if (token == tokenToIndex["USDT"]) {
            return true;
        }

        return false;
    }

    double Graph::calProfit(Strategy& strategy, vector<TransactionPathItem>& path)
    {
        double profit = 1; // 验算利润率
        for (const auto& item:path) {
            if (item.Side == define::SELL) {
                profit = profit*item.Price*(1-strategy.Fee);
            } else {
                profit = profit*(1/item.Price)*(1-strategy.Fee);
            }
        }

        return profit;
    }
}