#include <functional>
#include "utils/utils.h"
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
                auto ec = tryRebalance(token, "USDT", freeAmount);
                if (ec > 0)
                {
                    // todo error日志处理
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
            auto ec = tryRebalance(delToken, addToken, balancePool[delToken] - basePool[delToken]);
            if (ec > 0)
            {
                // todo error日志处理
                return;
            }
        }

        // 多余token换为USDT
        if (addToken.empty() && not delToken.empty())
        {
            auto ec = tryRebalance(delToken, "USDT", balancePool[delToken] - basePool[delToken]);
            if (ec > 0)
            {
                // todo error日志处理
                return;
            }
        }

        // 需补充资金
        if (not addToken.empty() && delToken.empty())
        {
            // todo 资金不足告警
        }
    }

    int CapitalPool::tryRebalance(string fromToken, string toToken, double amount)
    {
        // todo ticketSize校验补充
        HttpWrapper::CreateOrderReq req;
        req.OrderId = GenerateId();
        req.FromToken = fromToken;
        req.FromPrice = 0; // todo 算法层获取最新价格
        req.FromQuantity = amount;
        req.ToToken = toToken;
        req.ToPrice = 0; // todo 算法层获取最新价格
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
            // todo error刷新中
            return 1;
        }

        if (not balancePool.count(token))
        {
            // todo error资金不足
            return 1;
        }

        if (balancePool[token] < amount)
        {
            // todo error资金不足
            return 1;
        }

        balancePool[token] = balancePool[token] - amount; 
        return 0;
    }

    int CapitalPool::FreeAsset(string token, double amount)
    {
        if (locked)
        {
            // todo error刷新中
            return 1;
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
            // todo error处理+重试
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