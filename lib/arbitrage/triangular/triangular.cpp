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
            if (order.OrderStatus != define::FILLED && order.OrderStatus != define::PARTIALLY_FILLED) {
                return false;
            }
        }
        return true;
    }

    int TriangularArbitrage::Finish(double finalQuantity) {
        spdlog::info("func: Finish, finalQuantity: {}, originQuantity: {}", finalQuantity, this->OriginQuantity);
        capitalPool.Refresh();
        this->subscriber();
        return 0;
    }

    void TriangularArbitrage::SubscribeFinish(function<void()> callback) {
        this->subscriber = callback;
    }

    int TriangularArbitrage::ExecuteTrans(Pathfinder::TransactionPathItem &path) {
        auto orderId = GenerateId();
        orderMap[orderId].OrderId = orderId;
        orderMap[orderId].FromToken = path.FromToken;
        orderMap[orderId].ToToken = path.ToToken;
        orderMap[orderId].OriginPrice = path.FromPrice;
        orderMap[orderId].OriginQuantity = path.FromQuantity;
        orderMap[orderId].OrderType = define::LIMIT;
        orderMap[orderId].TimeInForce = define::IOC;
        orderMap[orderId].OrderStatus = define::INIT;
        orderMap[orderId].UpdateTime = GetNowTime();

        HttpWrapper::CreateOrderReq req;
        req.OrderId = orderId;
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

        // todo 这里深度不够需要重新找别的路径
        // todo 深度处理有bug
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
            spdlog::error("func: LockAsset, err: {}", WrapErr(err));
            return;
        }

        HttpWrapper::OrderData *order = &orderMap[data.OrderId];
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
        order->ExecutePrice = data.ExecutePrice;
        order->ExecuteQuantity = data.ExecuteQuantity;
        order->UpdateTime = data.UpdateTime;

        TransHandler(orderMap[data.OrderId]);
    }

    void TriangularArbitrage::TransHandler(HttpWrapper::OrderData &orderData) {
        exit(EXIT_FAILURE);
    }
}