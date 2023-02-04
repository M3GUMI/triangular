#include <functional>
#include "utils/utils.h"
#include "lib/pathfinder/pathfinder.h"
#include "capital_pool.h"

namespace CapitalPool
{
    CapitalPool capitalPool;
    CapitalPool &GetCapitalPool()
    {
        return capitalPool;
    }

    CapitalPool::CapitalPool()
    {
        vector<pair<string, double>> assets; // todo define获取
        for (auto asset : assets)
        {
            basePool[asset.first] = asset.second;
        }

        Refresh();
    }

    CapitalPool::~CapitalPool()
    {
    }

    void CapitalPool::RebalancePool()
    {
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
        if (addToken.empty() && not delToken.empty())
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
            LogError("func", "RebalancePool", "err", "need more asset");
            return;
        }
    }

    int CapitalPool::tryRebalance(string fromToken, string toToken, double amount)
    {
        Pathfinder::GetExchangePriceReq priceReq;
        priceReq.FromToken = fromToken;
        priceReq.ToToken = toToken;

        Pathfinder::GetExchangePriceResp priceResp;
        if (auto err = Pathfinder::GetPathfinder().GetExchangePrice(priceReq, priceResp); err > 0) {
            return err;
        }

        // todo ticketSize校验补充
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

        return API::GetBinanceApiWrapper().CreateOrder(req, bind(&CapitalPool::rebalanceHandler, this, placeholders::_1));
    }

    void CapitalPool::rebalanceHandler(HttpWrapper::OrderData &data)
    {
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
        API::GetBinanceApiWrapper().GetAccountInfo(bind(&CapitalPool::refreshAccountHandler, this, placeholders::_1, placeholders::_2));
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
        return;
    }
}