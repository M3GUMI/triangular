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

    // todo 重平衡最好改成maker，考虑maker执行中的锁定情况
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
                double toDollar = pathfinder.get2Dollar(token);
                if (toDollar == 0){
                    spdlog::info("func: {}, token: {}, err: {}", "RebalancePool", token, "Cannot transfer to dollar");
                    return;
                }

                double amount = balancePool[token];
                auto symbolData = apiWrapper.GetSymbolData(token, conf::BaseAsset);
                // 数量太少不管了
                if (amount * toDollar < 1){
                    continue;
                }
                else if (amount * toDollar < symbolData.MinNotional) {
                    Pathfinder::GetExchangePriceReq req;
                    req.BaseToken = symbolData.BaseToken;
                    req.QuoteToken = symbolData.QuoteToken;
                    req.OrderType = define::LIMIT;

                    Pathfinder::GetExchangePriceResp res;
                    auto err = pathfinder.GetExchangePrice(req, res);
                    if (err > 0) {
                        spdlog::info("{}, err: {}", "RebalancePool", WrapErr(err));
                        return;
                    }

                    double difDollar = symbolData.MinNotional + 0.002 - amount * toDollar;
                    double tokenQuantity = difDollar / toDollar;
                    // token 为计价币
                    if (symbolData.BaseToken == token){
                        OrderData data = {
                                .BaseToken = symbolData.BaseToken,
                                .QuoteToken = symbolData.QuoteToken,
                                .Price = res.SellPrice,
                                .Quantity = tokenQuantity,
                                .Side = define::SELL,
                                .OrderType = define::LIMIT,
                                .TimeInForce = define::GTC
                        };
                        apiWrapper.CreateOrder(data, nullptr);
                    }
                    // token 不为计价币
                    else if (symbolData.QuoteToken == token) {
                        OrderData data = {
                                .BaseToken = symbolData.BaseToken,
                                .QuoteToken = symbolData.QuoteToken,
                                .Price = res.BuyPrice,
                                .Quantity = tokenQuantity / res.BuyPrice,
                                .Side = define::BUY,
                                .OrderType = define::LIMIT,
                                .TimeInForce = define::GTC
                        };
                        apiWrapper.CreateOrder(data, nullptr);
                    }
                    spdlog::debug("func: {}, token: {}, baseToken: {}, tokenQuantity: {}",
                            "RebalancePool",
                            token,
                            symbolData.BaseToken,
                            tokenQuantity);
                }
                // 满足最小交易额 重平衡
                else {
                    auto err = tryRebalance(token, conf::BaseAsset, freeAmount);
                    if (err > 0 && err != define::ErrorLessTicketSize && err != define::ErrorLessMinNotional)
                    {
                        // spdlog::debug("func: RebalancePool, err: {}", WrapErr(err));
                    }
                }
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
            auto err = tryRebalance(delToken, addToken, balancePool[delToken] - basePool[delToken]);
            if (err > 0 && err != define::ErrorLessTicketSize)
            {
                spdlog::error("func: {}, err: {}", "RebalancePool", WrapErr(err));
                return;
            }
        }

        // 多余token换为USDT
        if (addToken.empty() && not delToken.empty())
        {
            auto err = tryRebalance(delToken, conf::BaseAsset, balancePool[delToken] - basePool[delToken]);
            if (err > 0)
            {
                spdlog::error("func: {}, err: {}", "RebalancePool", WrapErr(err));
                return;
            }
        }

        // 需补充资金
        if (not addToken.empty() && delToken.empty())
        {
            // spdlog::debug("func: {}, err: {}", "RebalancePool", "need more money");
            return;
        }
    }

    int CapitalPool::tryRebalance(const string& fromToken, const string& toToken, double amount)
    {
        auto symbolData = apiWrapper.GetSymbolData(fromToken, toToken);
        auto side = apiWrapper.GetSide(fromToken, toToken);
        define::OrderType orderType = define::LIMIT_MAKER;
        define::TimeInForce timeInForce = define::GTC;

        // 没有波动的稳定币，用taker
        //if (symbolData.Symbol == "USDCBUSD" || symbolData.Symbol == "BUSDUSDT") {
            orderType = define::LIMIT;
            timeInForce = define::IOC;
        //}

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

    void CapitalPool::rebalanceHandler(OrderData &data) {
        if (locked) {
            // 刷新期间不执行
            spdlog::debug("func: {}, msg: {}", "rebalanceHandler", "refresh execute, ignore");
            return;
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
                info.push_back(to_string(item.second));
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

        if (not balancePool.count(token) || balancePool[token] == 0) {
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
            Refresh();
            return;
        }

        double amount = 0;
        this->balancePool = {};
        for (const auto& asset : info.Balances)
        {
            if (asset.Free > 0) {
                if (asset.Token == "BUSD" || asset.Token == "USDT") {
                    amount += asset.Free;
                }
                this->balancePool[asset.Token] = asset.Free;
            }
        }

        locked = false;
        // todo 格式化输出一下balancePool
        spdlog::info("func: refreshAccountHandler, amount: {}, msg: refresh account success", amount);
    }
}