#include "utils/utils.h"
#include "capital_pool.h"

namespace CapitalPool
{
    // 0. 初始化账号余额，并rebalance
    // 1. 开始套利
    //   a. 资金池扣减token
    //   b. 建仓并锁定
    // 2. 交易失败部分释放，回归资金池并自动rebalance
    // 3. 套利执行中，盈利部分token资金池不感知
    // 4. 套利结束后，清仓。所有token回滚资金池
    // 定期比对资金池与账号余额差值，旁路校验bug

    CapitalPool::CapitalPool(vector<Asset> assets)
    {
        for(auto asset: assets) {
            basePool[asset.Token] = asset.Amount;
        }
        // 拉用户余额，初始化
        // balancePool
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
        req.OrderId = GenerateOrderId();
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

    int CapitalPool::CreatePosition(Asset& asset, Position& position)
    {
        auto token = asset.Token;
        auto amount = asset.Amount;

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

        Asset newAsset;
        newAsset.Token = token;
        newAsset.Amount = amount;

        position.PositionId = GeneratePositionId();
        position.AssetMap[token] = newAsset;
        positionMap[position.PositionId] = position;

        balancePool[token] = balancePool[token] - amount; 
        return 0;
    }

    int CapitalPool::GetPosition(string id, Position &position)
    {
        if (not positionMap.count(id)) {
            // todo error不存在
            return 1;
        }

        position.PositionId = positionMap[id].PositionId;
        position.AssetMap = positionMap[id].AssetMap;
        return 0;
    }

    int CapitalPool::FreePosition(string id, Asset &asset)
    {
        if (not positionMap.count(id))
        {
            // todo error不存在
            return 1;
        }

        auto token = asset.Token;
        auto amount = asset.Amount;
        auto position = positionMap[id];
        auto assetMap = position.AssetMap;
        if (not assetMap.count(token))
        {
            // todo 资产不存在
            return 1;
        }
        if (assetMap[token].Amount < amount)
        {
            // todo 资产不足
            return 1;
        }

        assetMap[token].Amount = assetMap[token].Amount - amount;
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

    int CapitalPool::EmptyPosition(string id)
    {
        if (not positionMap.count(id))
        {
            // todo error不存在
            return 1;
        }

        auto position = positionMap[id];
        for (auto item : position.AssetMap)
        {
            auto token = item.first;
            auto ec = FreePosition(id, position.AssetMap[token]);
            if (ec > 0)
            {
                // todo error处理
                return ec;
            }
        }

        positionMap.erase(id);
    }
}