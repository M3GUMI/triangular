#include "triangular.h"
#include "utils/utils.h"

namespace Arbitrage{
    TriangularArbitrage::TriangularArbitrage(
            Pathfinder::Pathfinder &pathfinder,
            CapitalPool::CapitalPool &pool,
            HttpWrapper::BinanceApiWrapper &apiWrapper
    ) : pathfinder(pathfinder), capitalPool(pool), apiWrapper(apiWrapper) {
    }

    TriangularArbitrage::~TriangularArbitrage() {
    }

    bool TriangularArbitrage::CheckFinish() {
        // 非目标币以外清空，套利完成
        for (const auto& item: balance) {
            auto token = item.first;
            auto quantity = item.second;

            if (token == this->TargetToken) {
                continue;
            }
            if (quantity > 0) {
                return false;
            }
        }

        return true;
    }

    int TriangularArbitrage::Finish(double finalQuantity) {
        spdlog::info("func: {}, finalQuantity: {}, originQuantity: {}", "Finish", finalQuantity, this->OriginQuantity);
        capitalPool.Refresh();
        this->subscriber();
        return 0;
    }

    void TriangularArbitrage::SubscribeFinish(function<void()> callback) {
        this->subscriber = callback;
    }

    int TriangularArbitrage::ExecuteTrans(Pathfinder::TransactionPathItem &path) {
        auto order = new HttpWrapper::OrderData();
        order->OrderId = GenerateId();
        order->FromToken = path.FromToken;
        order->ToToken = path.ToToken;
        order->OriginPrice = path.FromPrice;
        order->OriginQuantity = path.FromQuantity;
        order->OrderType = define::LIMIT;
        order->TimeInForce = define::IOC;
        order->OrderStatus = define::INIT;
        order->UpdateTime = GetNowTime();

        HttpWrapper::CreateOrderReq req;
        req.OrderId = order->OrderId;
        req.FromToken = path.FromToken;
        req.FromPrice = path.FromPrice;
        req.FromQuantity = path.FromQuantity;
        req.ToToken = path.ToToken;
        req.ToPrice = path.ToPrice;
        req.ToQuantity = path.ToQuantity;
        req.OrderType = define::LIMIT;
        req.TimeInForce = define::IOC;

        spdlog::info(
                "func: {}, from: {}, to: {}, price: {}, quantity: {}",
                "ExecuteTrans",
                path.FromToken,
                path.ToToken,
                path.FromPrice,
                path.FromQuantity
        );
        return apiWrapper.CreateOrder(
                req,
                bind(
                        &TriangularArbitrage::baseOrderHandler,
                        this,
                        placeholders::_1,
                        placeholders::_2
                ));
    }

    int TriangularArbitrage::ReviseTrans(string origin, string end, double quantity) {
        // 寻找新路径重试
        Pathfinder::RevisePathReq req;
        Pathfinder::ArbitrageChance resp;

        req.Origin = origin;
        req.End = end;
        req.PositionQuantity = quantity;

        auto err = pathfinder.RevisePath(req, resp);
        if (err > 0) {
            return err;
        }

        err = ExecuteTrans(resp.Path.front());
        if (err > 0) {
            return err;
        }
    }


    void TriangularArbitrage::baseOrderHandler(HttpWrapper::OrderData &data, int err) {
        if (err > 0) {
            spdlog::error("func: {}, err: {}", "LockAsset", WrapErr(err));
            return;
        }

        auto order = orderMap[data.OrderId];
        if (order.UpdateTime > data.UpdateTime) {
            return;
        }

        order.OrderStatus = data.OrderStatus;
        order.ExecutePrice = data.ExecutePrice;
        order.ExecuteQuantity = data.ExecuteQuantity;
        order.UpdateTime = data.UpdateTime;

        AddBalance(order.ToToken, order.ExecuteQuantity * order.ExecutePrice);
        DelBalance(order.FromToken, order.ExecuteQuantity);

        this->transHandler(order);
    }

    void TriangularArbitrage::AddBalance(string token, double amount) {
        balance[token] = balance[token] + amount;
    }

    void TriangularArbitrage::DelBalance(string token, double amount) {
        if (balance[token] < amount) {
            spdlog::critical(
                    "func: {}, err: {}, balanceQuantity: {}, executeQuantity: {}",
                    "baseOrderHandler",
                    "invalid quantity",
                    balance[token],
                   amount
            );
            exit(EXIT_FAILURE);
        }

        balance[token] = balance[token] - amount;
    }
}