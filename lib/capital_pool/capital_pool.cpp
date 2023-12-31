#include <functional>
#include "utils/utils.h"
#include "conf/conf.h"
#include "capital_pool.h"

namespace CapitalPool
{
    CapitalPool::CapitalPool(websocketpp::lib::asio::io_service &ioService, Pathfinder::Pathfinder &pathfinder, HttpWrapper::BinanceApiWrapper &api) : ioService(ioService), pathfinder(pathfinder), apiWrapper(api)
    {
        vector<pair<string, double>> assets;
        for (auto asset : conf::BaseAssets)
        {
            basePool[asset.first] = asset.second;
        }

        // 交易对数据就绪，开始rebalance
        apiWrapper.SubscribeSymbolReady(bind(&CapitalPool::RebalancePool, this, placeholders::_1));
        Refresh();
    }

    CapitalPool::~CapitalPool()
    {
    }

    void CapitalPool::RebalancePool(vector<HttpWrapper::BinanceSymbolData> &data)
    {
        // 每秒执行一次重平衡
        rebalanceTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService, websocketpp::lib::asio::milliseconds(5 * 1000));
        rebalanceTimer->async_wait(bind(&CapitalPool::RebalancePool, this, data));

        if (locked)
        {
            // 刷新期间不执行
            spdlog::debug("func: {}, msg: {}", "RebalancePool", "refresh execute, ignore");
            return;
        }

        for (auto iter = reBalanceOrderMap.begin(); iter != reBalanceOrderMap.end(); ++iter) {
            uint64_t orderID = iter->first;
            OrderData* order = iter->second;
            Pathfinder::GetExchangePriceReq priceReq{};
            priceReq.BaseToken = order->BaseToken;
            priceReq.QuoteToken = order->QuoteToken;
            priceReq.OrderType = order->OrderType;

            Pathfinder::GetExchangePriceResp priceResp{};
            if (auto err = pathfinder.GetExchangePrice(priceReq, priceResp); err > 0) {
                spdlog::debug("func::RebalancePool, err:{}", WrapErr(err));
                continue;
            }

            auto symbolData = apiWrapper.GetSymbolData(order->BaseToken, order->QuoteToken);
            bool needReOrder = false;
            if (order->Side == define::SELL && order->Price - priceResp.SellPrice > symbolData.StepSize){
                needReOrder = true;
            }
            if (order->Side == define::BUY && priceResp.BuyPrice - order->Price > symbolData.StepSize){
                needReOrder = true;
            }

            if (needReOrder && reBalanceOrderMap.count(orderID) &&
                    (reBalanceOrderMap[orderID]->OrderStatus == define::INIT ||
                    reBalanceOrderMap[orderID]->OrderStatus == define::NEW)){
                tryRebalance(orderID, order->GetFromToken(), order->GetToToken(), order->Quantity);

                spdlog::info("func::RebalancePool, msg: cancel and rebalanceorder, from:{}, to:{}", order->GetFromToken(), order->GetToToken());
            }
        }
        string addToken, delToken;
        double minPercent = 100, maxPercent = 0;

        for (const auto &item : balancePool)
        {
            auto token = item.first;
            auto freeAmount = item.second;
            if (lockedBalance.count(token) && lockedBalance[token]) {
                spdlog::debug("func: RebalancePool, token: {}, msg: rebalance now", token);
                continue;
            }

            if (freeAmount == 0) {
                continue;
            }

            // 非初始稳定币，清仓
            if (not basePool.count(token))
            {
                // u计价
                double dollarPrice = pathfinder.DollarPrice(token);
                if (dollarPrice == 0){
                    continue;
                }

                // 小于1刀不管
                double dollarAmount = dollarPrice*freeAmount;
                if (dollarAmount < 1){
                    continue;
                }

                auto symbolData = apiWrapper.GetSymbolData(token, conf::BaseAsset);
                // 重平衡
                if (dollarAmount > symbolData.MinNotional + 1) {
                    spdlog::info("func: RebalancePool, token: {}, dollarAmount: {}", token, dollarAmount);
                    auto err = tryRebalance(0, token, conf::BaseAsset, freeAmount);
                    if (err > 0 && err != define::ErrorLessTicketSize && err != define::ErrorLessMinNotional && err != define::ErrorGraphNotReady)
                    {
                        spdlog::error("func: RebalancePool, err: {}", WrapErr(err));
                    }
                    continue;
                }

                // 补充u
                /*if (dollarAmount < symbolData.MinNotional) {
                    double addDollar = symbolData.MinNotional + 1;
                    double tokenQuantity = addDollar / dollarPrice;
                    OrderData req{
                        .OrderId = GenerateId(),
                        .BaseToken = symbolData.BaseToken,
                        .QuoteToken = symbolData.QuoteToken,
                        .OrderType = define::MARKET
                    };
                    if (symbolData.BaseToken == token){
                        req.Price = dollarPrice;
                        req.Quantity = tokenQuantity;
                        req.Side = define::BUY;
                    } else {
                        req.Price = double(1) / dollarPrice;
                        req.Quantity = addDollar;
                        req.Side = define::SELL;
                    }

                    spdlog::info("func: RebalancePool, msg: add position to rebalance, token: {}, dollar: {}, addDollar: {}", token, dollarAmount, addDollar);
                    auto apiCallback = bind(&CapitalPool::rebalanceHandler, this, placeholders::_1);

                    apiWrapper.CreateOrder(req, apiCallback);
                }*/
                continue;
            }
        }

        // 获取需补充的token
        for (const auto &item : basePool)
        {
            auto token = item.first;
            double freeAmount = 0;
            if (lockedBalance.count(token) && lockedBalance[token]) {
                spdlog::debug("func: RebalancePool, token: {}, msg: rebalance now", token);
                continue;
            }

            if (balancePool.count(token))
            {
                freeAmount = balancePool[token];
            }

            if (freeAmount < basePool[token] && freeAmount / basePool[token] < minPercent)
            {
                addToken = token;
                minPercent = freeAmount / basePool[token];
            }
        }

        // 获取需减少的token
        for (const auto &item : basePool)
        {
            auto token = item.first;
            double freeAmount = 0;
            if (balancePool.count(token))
            {
                freeAmount = balancePool[token];
            }

            if (addToken.empty() && token == conf::BaseAsset)  {
                // 无需补充时，不清仓USDT
                continue;
            }

            if (freeAmount > basePool[token] && freeAmount / basePool[token] > maxPercent)
            {
                delToken = token;
                maxPercent = freeAmount / basePool[token];
            }
        }

        // 交换两种token
        if (not addToken.empty() && not delToken.empty())
        {
            auto err = tryRebalance(0, delToken, addToken, balancePool[delToken] - basePool[delToken]);
            if (err > 0 && err != define::ErrorLessTicketSize)
            {
                spdlog::error("func: {}, err: {}", "RebalancePool", WrapErr(err));
                return;
            }
        }

        // 多余token换为USDT
        if (addToken.empty() && not delToken.empty())
        {
            auto err = tryRebalance(0, delToken, conf::BaseAsset, balancePool[delToken] - basePool[delToken]);
            if (err > 0)
            {
                spdlog::error("func: {}, err: {}", "RebalancePool", WrapErr(err));
                return;
            }
        }

        // 需补充资金
        if (not addToken.empty() && delToken.empty())
        {
            return;
        }
    }

    int CapitalPool::tryRebalance(uint64_t cancelOrderId, const string& fromToken, const string& toToken, double amount)
    {
        auto symbolData = apiWrapper.GetSymbolData(fromToken, toToken);
        auto side = apiWrapper.GetSide(fromToken, toToken);
        define::OrderType orderType = define::MARKET;
        define::TimeInForce timeInForce = define::GTC;

        Pathfinder::GetExchangePriceReq priceReq{};
        priceReq.BaseToken = symbolData.BaseToken;
        priceReq.QuoteToken = symbolData.QuoteToken;
        priceReq.OrderType = orderType;

        Pathfinder::GetExchangePriceResp priceResp{};
        if (auto err = pathfinder.GetExchangePrice(priceReq, priceResp); err > 0) {
            return err;
        }

        OrderData order;
        order.OrderId = GenerateId();
        order.CancelOrderId = cancelOrderId;
        order.BaseToken = symbolData.BaseToken;
        order.QuoteToken = symbolData.QuoteToken;
        order.Side = side;
        order.OrderType = orderType;
        order.TimeInForce = timeInForce;

        if (side == define::SELL) {
            order.Price = priceResp.SellPrice;
            order.Quantity = amount <= priceResp.SellQuantity? amount: priceResp.SellQuantity;
        } else {
            order.Price = priceResp.BuyPrice;
            amount = RoundDouble(amount/order.Price);
            order.Quantity = amount <= priceResp.BuyQuantity? amount: priceResp.BuyQuantity;
        }
        auto err = apiWrapper.CreateOrder(order, bind(&CapitalPool::rebalanceHandler, this, placeholders::_1));
        if (err > 0) {
            return err;
        }

        lockedBalance[fromToken] = true;
        return 0;
    }

    void CapitalPool::cancelHandler(int err, int orderId) {
        if (err == 0 && reBalanceOrderMap.count(orderId)) {
            reBalanceOrderMap[orderId]->OrderStatus = define::CANCELED;
        }
    }

    void CapitalPool::rebalanceHandler(OrderData &data) {
        if (locked) {
            // 刷新期间不执行
            spdlog::debug("func: {}, msg: {}", "rebalanceHandler", "refresh execute, ignore");
            return;
        }

        if (reBalanceOrderMap.count(data.OrderId)) {
            reBalanceOrderMap[data.OrderId]->OrderStatus = data.OrderStatus;
        } else {
            auto order = new OrderData{data};
            reBalanceOrderMap[data.OrderId] = order;
        }

        if (not balancePool.count(data.GetFromToken())) {
            spdlog::error("func: {}, err: {}", "rebalanceHandler", WrapErr(define::ErrorBalanceNumber));
            balancePool[data.GetFromToken()] = 0;
        } else if (balancePool[data.GetFromToken()] < data.GetExecuteQuantity()) {
            spdlog::error("func: {}, err: {}", "rebalanceHandler", WrapErr(define::ErrorBalanceNumber));
            balancePool[data.GetFromToken()] = 0;
        } else {
            balancePool[data.GetFromToken()] = balancePool[data.GetFromToken()] - data.GetExecuteQuantity();
        }

        if (not balancePool.count(data.GetToToken())) {
            balancePool[data.GetToToken()] = data.GetNewQuantity();
        } else {
            balancePool[data.GetToToken()] = balancePool[data.GetToToken()] + data.GetNewQuantity();
        }

        lockedBalance[data.GetFromToken()] = false;
        vector<string> info;
        for (const auto &item: balancePool) {
            if (not item.first.empty()) {
                info.push_back(item.first);
                info.push_back(to_string(pathfinder.DollarPrice(item.first)*item.second));
            }
        }
        spdlog::info("func: {}, balancePool: {}", "rebalanceHandler", spdlog::fmt_lib::join(info, ","));
    }

    int CapitalPool::LockAsset(const string& token, double amount, double& lockAmount) {
        if (locked) {
            spdlog::error(
                    "func: LockAsset, token: {}, amount: {}, err: {}",
                    token,
                    amount,
                    WrapErr(define::ErrorCapitalRefresh));
            return define::ErrorCapitalRefresh;
        }

        // todo 先写死，后面改
        if (not balancePool.count(token) || balancePool[token] < 15) {
            spdlog::error(
                    "func: LockAsset, token: {}, amount: {}, err: {}",
                    token,
                    amount,
                    WrapErr(define::ErrorInsufficientBalance));
            return define::ErrorInsufficientBalance;
        }

        if (balancePool[token] < amount) {
            lockAmount = balancePool[token];
            balancePool[token] = 0;
            return 0;
        }

        lockAmount = amount;
        balancePool[token] = balancePool[token] - amount;
        return 0;
    }

    int CapitalPool::FreeAsset(const string& token, double amount) {
        if (locked) {
            spdlog::error("func: FreeAsset, err: {}", WrapErr(define::ErrorCapitalRefresh));
            return define::ErrorCapitalRefresh;
        }

        if (balancePool.count(token)) {
            balancePool[token] = balancePool[token] + amount;
        } else {
            balancePool[token] = amount;
        }

        return 0;
    }

    int CapitalPool::Refresh()
    {
        locked = true;
        // todo 取消所有订单
        apiWrapper.GetAccountInfo(bind(&CapitalPool::refreshAccountHandler, this, placeholders::_1, placeholders::_2));
        return 0;
    }

    void CapitalPool::refreshAccountHandler(HttpWrapper::AccountInfo &info, int err)
    {
        if (err > 0)
        {
            spdlog::error("func: {}, err: {}", "refreshAccountHandler", WrapErr(err));
            // Refresh();
            return;
        }

        double xrpAmount = 0;
        double usdtAmount = 0;
        double busdAmount = 0;
        this->balancePool = {};
        for (const auto& asset : info.Balances)
        {
            if (asset.Free > 0) {
                if (asset.Token == "XRP") {
                    xrpAmount = asset.Free;
                }
                if (asset.Token == "USDT") {
                    usdtAmount = asset.Free;
                }
                if (asset.Token == "BUSD") {
                    busdAmount = asset.Free;
                }
                this->balancePool[asset.Token] = asset.Free;
            }
        }

        locked = false;
        // todo 格式化输出一下balancePool
        spdlog::info("func: refreshAccountHandler, usdt: {}, busdt: {}, xrp: {}, msg: refresh account success", usdtAmount, busdAmount, xrpAmount);
    }
}