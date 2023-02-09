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

    // todo 重平衡逻辑有误，需要考虑maker执行中情况
    void CapitalPool::RebalancePool(map<string, HttpWrapper::BinanceSymbolData> &data)
    {
        // 每秒执行一次重平衡
        rebalanceTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService, websocketpp::lib::asio::milliseconds(5 * 1000));
        rebalanceTimer->async_wait(bind(&CapitalPool::RebalancePool, this, data));

        if (locked)
        {
            // 刷新期间不执行
            LogDebug("func", "RebalancePool", "msg", "refresh execute, ignore");
            return;
        }

        string addToken, delToken;
        double minPercent = 100, maxPercent = 0;

        for (const auto &item : balancePool)
        {
            auto token = item.first;
            auto freeAmount = item.second;

            // 非初始稳定币，清仓
            if (not basePool.count(token))
            {
                auto err = tryRebalance(token, "USDT", freeAmount);
                if (err > 0)
                {
                    LogError("func", "RebalancePool", "err", WrapErr(err));
                    return;
                }
                continue;
            }
        }

        for (const auto &item : basePool)
        {
            auto token = item.first;
            double freeAmount = 0;
            if (balancePool.count(token))
            {
                freeAmount = balancePool[token];
            }

            // 获取偏差比例最大的两种token
            if (freeAmount < basePool[token] && freeAmount / basePool[token] < minPercent)
            {
                addToken = token;
                minPercent = freeAmount / basePool[token];
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
            if (err > 0)
            {
                LogError("func", "RebalancePool", "err", WrapErr(err));
                return;
            }
        }

        // 多余token换为USDT
        if (addToken.empty() && not delToken.empty() && delToken != "USDT")
        {
            auto err = tryRebalance(delToken, "USDT", balancePool[delToken] - basePool[delToken]);
            if (err > 0)
            {
                LogError("func", "RebalancePool", "err", WrapErr(err));
                return;
            }
        }

        // 需补充资金
        if (not addToken.empty() && delToken.empty())
        {
            LogError("func", "RebalancePool", "err", "need more money");
            return;
        }
    }

    int CapitalPool::tryRebalance(string fromToken, string toToken, double amount)
    {
        Pathfinder::GetExchangePriceReq priceReq;
        priceReq.FromToken = fromToken;
        priceReq.ToToken = toToken;

        Pathfinder::GetExchangePriceResp priceResp;
        if (auto err = pathfinder.GetExchangePrice(priceReq, priceResp); err > 0) {
            return err;
        }

        // ticketSize校验
        auto data = this->apiWrapper.GetSymbolData(fromToken, toToken);
        double ticketSize = (data.TicketSize == 0 ? 1 : data.TicketSize);
        uint32_t tmp = amount / ticketSize;
        amount = tmp * ticketSize;

        HttpWrapper::CreateOrderReq req;
        req.OrderId = GenerateId();
        req.FromToken = fromToken;
        req.FromPrice = priceResp.FromPrice;
        req.FromQuantity = amount;
        req.ToToken = toToken;
        req.ToPrice = priceResp.ToPrice;
        req.ToQuantity = req.FromPrice * req.FromQuantity;
        req.OrderType = define::LIMIT;
        req.TimeInForce = define::IOC;

        return apiWrapper.CreateOrder(req, bind(&CapitalPool::rebalanceHandler, this, placeholders::_1));
    }

    void CapitalPool::rebalanceHandler(HttpWrapper::OrderData &data)
    {
        if (locked)
        {
            // 刷新期间不执行
            LogDebug("func", "rebalanceHandler", "msg", "refresh execute, ignore");
            return;
        }

        if (not balancePool.count(data.FromToken))
        {
            LogError("func", "rebalanceHandler", "err", WrapErr(ErrorBalanceNumber));
            balancePool[data.FromToken] = 0;
        }
        else if (balancePool[data.FromToken] < data.ExecuteQuantity)
        {
            LogError("func", "rebalanceHandler", "err", WrapErr(ErrorBalanceNumber));
            balancePool[data.FromToken] = 0;
        }
        else
        {
            balancePool[data.FromToken] = balancePool[data.FromToken] - data.ExecuteQuantity;
        }

        if (not balancePool.count(data.ToToken))
        {
            balancePool[data.ToToken] = data.ExecuteQuantity * data.ExecutePrice;
        }
        else
        {
            balancePool[data.ToToken] = balancePool[data.ToToken] + data.ExecuteQuantity * data.ExecutePrice;
        }

        // todo 临时这样打日志
        cout << "[Info]RebalancePool: ";
        for (auto item : balancePool)
        {
            cout << item.first << " " << to_string(item.second) << ", ";
        }
        cout << endl;
    }

    int CapitalPool::LockAsset(string token, double amount)
    {
        if (locked) {
            LogError("func", "LockAsset", "err", WrapErr(define::ErrorCapitalRefresh));
            return define::ErrorCapitalRefresh;
        }

        if (not balancePool.count(token))
        {
            LogError("func", "LockAsset", "err", WrapErr(define::ErrorInsufficientBalance));
            return define::ErrorInsufficientBalance;
        }

        if (balancePool[token] < amount)
        {
            LogError("func", "LockAsset", "err", WrapErr(define::ErrorInsufficientBalance));
            return define::ErrorInsufficientBalance;
        }

        balancePool[token] = balancePool[token] - amount; 
        return 0;
    }

    int CapitalPool::FreeAsset(string token, double amount)
    {
        if (locked)
        {
            LogError("func", "LockAsset", "err", WrapErr(define::ErrorCapitalRefresh));
            return define::ErrorCapitalRefresh;
        }

        if (balancePool.count(token))
        {
            balancePool[token] = balancePool[token] + amount;
        }
        else
        {
            balancePool[token] = amount;
        }

        return 0;
    }

    int CapitalPool::Refresh()
    {
        locked = true;
        // todo 取消所有订单
        apiWrapper.GetAccountInfo(bind(&CapitalPool::refreshAccountHandler, this, placeholders::_1, placeholders::_2));
    }

    void CapitalPool::refreshAccountHandler(HttpWrapper::AccountInfo &info, int err)
    {
        if (err > 0)
        {
            LogError("func", "refreshAccountHandler", "err", WrapErr(err));
            Refresh();
            return;
        }

        for (auto asset : info.Balances)
        {
            this->balancePool = {};
            this->balancePool[asset.Token] = asset.Free;
        }

        locked = false;
        LogInfo("func", "refreshAccountHandler", "msg", "refresh account success");
        return;
    }
}