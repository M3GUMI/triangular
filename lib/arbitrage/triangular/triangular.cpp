#include "triangular.h"

namespace Arbitrage{
    TriangularArbitrage::TriangularArbitrage(
            Pathfinder::Pathfinder &pathfinder,
            CapitalPool::CapitalPool &pool,
            HttpWrapper::BinanceApiWrapper &apiWrapper
    ) : pathfinder(pathfinder), capitalPool(pool), apiWrapper(apiWrapper) {
    }

    TriangularArbitrage::~TriangularArbitrage() = default;


    int TriangularArbitrage::Run(Pathfinder::ArbitrageChance &chance) {
        exit(EXIT_FAILURE);
    }

    bool TriangularArbitrage::CheckFinish() {
        for (const auto &item: orderMap) {
            auto order = item.second;
            if (order->OrderStatus != define::FILLED && order->OrderStatus != define::PARTIALLY_FILLED) {
                return false;
            }
        }
        return true;
    }

    int TriangularArbitrage::Finish(double finalQuantity) {
        spdlog::info("func: Finish, profit: {}, finalQuantity: {}, originQuantity: {}", finalQuantity / this->OriginQuantity, finalQuantity, this->OriginQuantity);
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
        order->FromPrice = path.FromPrice;
        order->FromQuantity = path.FromQuantity;
        order->ToToken = path.ToToken;
        order->ToPrice = path.ToPrice;
        order->ToQuantity = path.ToQuantity;
        order->OrderType = define::LIMIT;
        order->TimeInForce = define::IOC;
        order->OrderStatus = define::INIT;
        order->UpdateTime = GetNowTime();
        orderMap[order->OrderId] = order;

        spdlog::info(
                "func: {}, from: {}, to: {}, price: {}, quantity: {}",
                "ExecuteTrans",
                path.FromToken,
                path.ToToken,
                path.FromPrice,
                path.FromQuantity
        );
        return apiWrapper.CreateOrder(
                *order,
                bind(
                        &TriangularArbitrage::baseOrderHandler,
                        this,
                        placeholders::_1,
                        placeholders::_2
                ));
    }

    int TriangularArbitrage::ReviseTrans(string origin, string end, double quantity) {
        // todo 此处有bug，depth不足的话，有一部分金额卡单了
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

        spdlog::info(
                "func: RevisePath, profit: {}, bestPath: {}",
                resp.Profit,
                spdlog::fmt_lib::join(resp.Format(), ","));

        if (resp.Path.front().FromQuantity > quantity) {
            resp.Path.front().FromQuantity = quantity;
        }

        err = ExecuteTrans(resp.FirstStep());
        if (err > 0) {
            return err;
        }
    }

    void TriangularArbitrage::baseOrderHandler(HttpWrapper::OrderData &data, int err) {
        if (err > 0) {
            spdlog::error("func: baseOrderHandler, err: {}", WrapErr(err));
            return;
        }

        if (not orderMap.count(data.OrderId)) {
            spdlog::error("func: baseOrderHandler, err: {}", "order not exist");
            return;
        }

        if (orderMap[data.OrderId] == nullptr) {
            spdlog::error("func: baseOrderHandler, err: {}", "order not exist");
            return;
        }

        HttpWrapper::OrderData* order = orderMap[data.OrderId];
        if (conf::EnableMock) { // mock情况下可相同毫秒更新
            if (order->UpdateTime > data.UpdateTime) {
                return;
            }
        } else {
            if (order->UpdateTime >= data.UpdateTime) {
                return;
            }
        }

        order->OrderStatus = data.OrderStatus;
        // todo 这里的origin改成校验逻辑
        order->OriginPrice = data.OriginPrice;
        order->OriginQuantity = data.OriginQuantity;
        order->ExecutePrice = data.ExecutePrice;
        order->ExecuteQuantity = data.ExecuteQuantity;
        order->UpdateTime = data.UpdateTime;

        TransHandler(*order);
    }

    void TriangularArbitrage::TransHandler(HttpWrapper::OrderData &orderData) {
        exit(EXIT_FAILURE);
    }
}