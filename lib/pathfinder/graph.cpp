#include <cfloat>
#include "graph.h"
#include <sys/timeb.h>
#include <sys/time.h>

namespace Pathfinder{
    Graph::Graph(HttpWrapper::BinanceApiWrapper &apiWrapper): apiWrapper(apiWrapper) {
    }
    Graph::~Graph() = default;

    void Graph::Init(vector<HttpWrapper::BinanceSymbolData> &data)
    {
        tokenToIndex.clear();
        indexToToken.clear();
        triangularMap.clear();
        bestPathMap.clear();
        nodeTriangularMap.clear();

        // 初始化index
        int indexCount = 0;
        for (auto& item : data)
        {
            if (not tokenToIndex.count(item.BaseToken)) {
                tokenToIndex[item.BaseToken] = indexCount;
                indexToToken[indexCount] = item.BaseToken;
                indexCount++;
            }

            if (not tokenToIndex.count(item.QuoteToken)) {
                tokenToIndex[item.QuoteToken] = indexCount;
                indexToToken[indexCount] = item.QuoteToken;
                indexCount++;
            }
        }

        // 初始化交易节点
        for (auto& item : data)
        {
            auto baseIndex = tokenToIndex[item.BaseToken];
            auto quoteIndex = tokenToIndex[item.QuoteToken];

            auto tradeNode = new Node(baseIndex, quoteIndex);
            this->tradeNodeMap[formatKey(baseIndex, quoteIndex)] = tradeNode;
            this->tradeNodeMap[formatKey(quoteIndex, baseIndex)] = tradeNode;
        }

        // 存储所有三元环
        int usdtIndex = tokenToIndex["USDT"];
        for (const auto &baseToken: conf::BaseAssets)
        {
            if (not tokenToIndex.count(baseToken.first))
            {
                spdlog::error("func: Graph::init, msg: baseAsset not exist");
                continue;
            };

            auto originIndex = tokenToIndex[baseToken.first]; // 起点token
            for (const auto& second : indexToToken)
            {
                auto secondIndex = second.first; // 第一个点
                if (not tradeNodeMap.count(formatKey(originIndex, secondIndex))) {
                    // 第一条边不存在
                    continue;
                }

                for (const auto& third : indexToToken)
                {
                    auto thirdIndex = third.first; // 第二个点
                    if (not tradeNodeMap.count(formatKey(secondIndex, thirdIndex))) {
                        // 第二条边不存在
                        continue;
                    }
                    if (not tradeNodeMap.count(formatKey(thirdIndex, originIndex))) {
                        // 第三条边不存在
                        continue;
                    }

                    if (secondIndex == usdtIndex || thirdIndex == usdtIndex) {
                        // 摘掉usdt，路径一般执行不完
                        continue;
                    }

                    triangularMap[originIndex].emplace_back(vector<int>{originIndex, secondIndex, thirdIndex, originIndex});
                    // 方向1
                    nodeTriangularMap[formatKey(originIndex, secondIndex)].emplace_back(vector<int>{originIndex, secondIndex, thirdIndex, originIndex});
                    // 方向2
                    nodeTriangularMap[formatKey(originIndex, secondIndex)].emplace_back(vector<int>{originIndex, thirdIndex, secondIndex, originIndex});
                }
            }
        }

        // 存储所有一步、两步路径
        for (const auto& origin : indexToToken)
        {
            auto originIndex = origin.first; // 起点
            for (const auto& second : indexToToken)
            {
                auto secondIndex = second.first; // 第二个点
                if (not tradeNodeMap.count(formatKey(originIndex, secondIndex))) {
                    // 第一条边不存在
                    continue;
                }

                // 存储一步路径
                bestPathMap[formatKey(originIndex, secondIndex)].emplace_back(vector<int>{originIndex, secondIndex});

                for (const auto& third : indexToToken)
                {
                    auto thirdIndex = third.first; // 第三个点
                    if (not tradeNodeMap.count(formatKey(secondIndex, thirdIndex)))
                    {
                        // 第二条边不存在
                        continue;
                    }

                    // 存储两步路径
                    bestPathMap[formatKey(originIndex, thirdIndex)].emplace_back(vector<int>{originIndex, secondIndex, thirdIndex});
                }
            }
        }
    }

    void Graph::UpdateNode(WebsocketWrapper::DepthData &data)
    {
        auto baseIndex = tokenToIndex[data.BaseToken];
        auto quoteIndex = tokenToIndex[data.QuoteToken];

        auto node = tradeNodeMap[formatKey(baseIndex, quoteIndex)];
        if (not data.Bids.empty())
        { // 买单挂出价，我方卖出价
            auto depth = data.Bids[0];
            node->UpdateSell(depth.Price, depth.Quantity);
        }
        if (!data.Asks.empty())
        { // 卖单挂出价，我方买入价
            auto depth = data.Asks[0];
            node->UpdateBuy(depth.Price, depth.Quantity);
        }

        auto chance = CalculateArbitrage("IocTriangular", baseIndex, quoteIndex);
        if (chance.Profit <= 1)
        {
            return;
        }

        spdlog::info("func: {}, path found profit: {}", "UpdateNode", chance.Profit);

        return this->subscriber(chance);
    }

    int Graph::GetExchangePrice(GetExchangePriceReq &req, GetExchangePriceResp &resp)
    {
        if (not tokenToIndex.count(req.BaseToken) || not tokenToIndex.count(req.QuoteToken)) {
            return define::ErrorGraphNotReady;
        }

        int baseIndex = tokenToIndex[req.BaseToken];
        int quoteIndex = tokenToIndex[req.QuoteToken];

        auto node = tradeNodeMap[formatKey(baseIndex, quoteIndex)];
        resp.SellPrice = node->GetOriginPrice(baseIndex, quoteIndex);
        resp.SellQuantity = node->GetQuantity(baseIndex, quoteIndex);
        resp.BuyPrice = node->GetOriginPrice(quoteIndex, baseIndex);
        resp.BuyQuantity = node->GetQuantity(quoteIndex, baseIndex);

        // maker不可下单到对手盘
        auto symbolData = apiWrapper.GetSymbolData(req.BaseToken, req.QuoteToken);
        if (req.OrderType == define::LIMIT_MAKER) {
            resp.SellPrice = resp.SellPrice + symbolData.TicketSize;
        }
        if (req.OrderType == define::LIMIT_MAKER) {
            resp.BuyPrice = resp.BuyPrice - symbolData.TicketSize;
        }

        if (resp.SellPrice == 0 || resp.BuyPrice == 0) {
            return define::ErrorGraphNotReady;
        }

        return 0;
    }

    ArbitrageChance Graph::CalculateArbitrage(const string& name, int baseIndex, int quoteIndex) {
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

        double maxProfit = 0;
        vector<TransactionPathItem> resultPath{};

        struct timeval tv1, tv2;
        gettimeofday(&tv1,NULL);

        // 三元环base到quote方向寻找套利机会
//        for (auto item: nodeTriangularMap[formatKey(baseIndex, quoteIndex)]){
//            spdlog::info("func: {}, before calculate profit, ring:{}->{}->{}", "CalculateArbitrage", item[0], item[1], item[2]);
//            double profit = calculateProfit(strategy, item);
//            spdlog::info("func: {}, after calculate profit, profit: {}, max profit: {}", "CalculateArbitrage", profit, maxProfit);
//            if (profit <= 1 || profit <= maxProfit)
//            {
//                continue;
//            }
//            auto path = formatPath(strategy, item);
//            adjustQuantities(path);
//            if (not checkPath(path))
//            {
//                continue;
//            }
//
//            maxProfit = profit;
//            resultPath = path;
//            spdlog::info("func: {}, update max profit, profit: {}, max profit: {}", "CalculateArbitrage", profit, maxProfit);
//        }

        // 三元环quote到base方向找套利机会
        for (auto item: nodeTriangularMap[formatKey(quoteIndex, baseIndex)]){
//            spdlog::info("func: {}, before calculate profit, ring:{}->{}->{}", "CalculateArbitrage", item[0], item[1], item[2]);
            double profit = calculateProfit(strategy, item);
//            spdlog::info("func: {}, after calculate profit, profit: {}, max profit: {}", "CalculateArbitrage", profit, maxProfit);
            if (profit <= 1 || profit <= maxProfit)
            {
                continue;
            }

            auto path = formatPath(strategy, item);
            adjustQuantities(path);
            if (not checkPath(path))
            {
                continue;
            }

            maxProfit = profit;
            resultPath = path;
            spdlog::info("func: {}, update max profit, profit: {}, max profit: {}", "CalculateArbitrage", profit, maxProfit);
        }
        gettimeofday(&tv2,NULL);

        spdlog::info("func: {}, scan first rings: {}, scan second rings: {}, path time cost: {}us, max profit: {}", "CalculateArbitrage",
                nodeTriangularMap[formatKey(baseIndex, quoteIndex)].size(),
                nodeTriangularMap[formatKey(quoteIndex, baseIndex)].size(),
                tv2.tv_sec*1000000 + tv2.tv_usec - (tv1.tv_sec*1000000 + tv1.tv_usec),
                maxProfit);

        ArbitrageChance chance{};
        if (resultPath.size() != 3) {
            return chance;
        }

        chance.Profit = maxProfit;
        chance.Quantity = resultPath.front().Quantity;
        chance.Path = resultPath;
        return chance;
    }

//    ArbitrageChance Graph::CalculateArbitrage(const string& name) {
//        Strategy strategy{};
//        if (name == "maker") {
//            strategy.Fee = 0;
//            strategy.OrderType = define::LIMIT_MAKER;
//            strategy.TimeInForce = define::GTC;
//        } else {
//            strategy.Fee = 0.0004;
//            strategy.OrderType = define::LIMIT;
//            strategy.TimeInForce = define::IOC;
//        }
//
//        double maxProfit = 0;
//        vector<TransactionPathItem> resultPath{};
//        for (const auto& item: triangularMap)
//        {
//            indexStart += groupSize;
//            if (indexStart > item.second.size()) {
//                indexStart = 0;
//            }
//
//            int indexEnd = indexStart+groupSize;
//            if (indexEnd > item.second.size()) {
//                indexEnd = int(item.second.size());
//            }
//
//            for (int i = indexStart; i < indexEnd; i++)
//            {
//                auto triangular = item.second[i];
//                double profit = calculateProfit(strategy, triangular);
//                if (profit <= 1 || profit <= maxProfit)
//                {
//                    continue;
//                }
//
//                auto path = formatPath(strategy, triangular);
//                adjustQuantities(path);
//                if (not checkPath(path))
//                {
//                    continue;
//                }
//
//                maxProfit = profit;
//                resultPath = path;
//            }
//        }
//
//        ArbitrageChance chance{};
//        if (resultPath.size() != 3) {
//            return chance;
//        }
//
//        chance.Profit = maxProfit;
//        chance.Quantity = resultPath.front().Quantity;
//        chance.Path = resultPath;
//        return chance;
//    }

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

        int originToken = tokenToIndex[req.Origin];
        int endToken = tokenToIndex[req.End];

        double maxProfit = 0;
        vector<TransactionPathItem> resultPath{};
        for (auto& item: bestPathMap[formatKey(originToken, endToken)])
        {
            double profit = calculateProfit(strategy, item);
            if (profit <= maxProfit) {
                continue;
            }

            auto path = formatPath(strategy, item);
            adjustQuantities(path);
            if (not checkPath(path)) {
                continue;
            }

            maxProfit = profit;
            resultPath = path;
        }

        ArbitrageChance chance{};
        if (resultPath.empty()) {
            return chance;
        }

        auto pathPrice = resultPath.front().Price;
        auto pathQuantity = resultPath.front().Quantity;

        auto quantity = req.Quantity;
        if (resultPath.front().Side == define::BUY) {
            quantity /= pathPrice; // 转换成baseToken数量
        }
        double realQuantity = RoundDouble(Min(quantity, pathQuantity));

        resultPath.front().Quantity = realQuantity;
        chance.Profit = maxProfit;
        chance.Quantity = realQuantity;
        chance.Path = resultPath;
        return chance;
    }

    vector<TransactionPathItem> Graph::formatPath(Strategy& strategy, vector<int>& path)
    {
        vector<TransactionPathItem> result;
        if (path.size() < 2)
        {
            return result;
        }

        for (int i = 0; i < path.size() - 1; i++)
        {
            auto from = path[i];
            auto to = path[i + 1];

            auto node = tradeNodeMap[formatKey(from, to)];
            result.emplace_back(node->Format(strategy, indexToToken, from, to));
        }

        return result;
    }

    bool Graph::checkPath(vector<TransactionPathItem>& path)
    {
        for (TransactionPathItem& item : path)
        {
            // 检查最小成交金额
            auto symbolData = apiWrapper.GetSymbolData(item.BaseToken, item.QuoteToken);
            if (symbolData.MinNotional > item.GetNationalQuantity())
            {
                return false;
            }
        }

        if (path.front().Quantity < conf::MinTriangularQuantity)
        {
            return false;
        }

        return true;
    }

    double Graph::calculateProfit(Strategy& strategy, vector<int>& path)
    {
        if (path.size() < 2)
        {
            return 0;
        }

        double profit = 1; // 验算利润率
        for (int i = 0; i < path.size() - 1; i++)
        {
            auto from = path[i];
            auto to = path[i + 1];

            auto node = tradeNodeMap[formatKey(from, to)];
            profit = profit * node->GetParsePrice(from, to) * (1 - strategy.Fee);
        }

        return profit;
    }

    u_int64_t Graph::formatKey(int from, int to)
    {
        u_int64_t result = u_int64_t(from) << 32 | to;
        return result;
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

    void Graph::SubscribeArbitrage(function<void(ArbitrageChance &path)> handler)
    {
        this->subscriber = handler;
    }
}