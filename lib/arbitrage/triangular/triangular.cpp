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
        for(auto order:orderMap) {
            auto orderStatus = order.second.OrderStatus;
            if (orderStatus != define::FILLED && orderStatus != define::PARTIALLY_FILLED) {
                cout << order.second.FromToken << endl;
                cout << order.second.ToToken << endl;
                cout << orderStatus << endl;
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

        vector<string> info;
        info.push_back(resp.Path[0].FromToken);
        for (const auto &item: resp.Path) {
            info.push_back(to_string(item.FromPrice));
            info.push_back(item.ToToken);
        }
        spdlog::info("func: RevisePath, Profit: {}, BestPath: {}", resp.Profit, spdlog::fmt_lib::join(info, ","));

        // todo 这里深度不够需要重新找别的路径
        if (resp.Path.front().FromQuantity > quantity) {
            resp.Path.front().FromQuantity = quantity;
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
        if (conf::EnableMock) {
            if (order.UpdateTime > data.UpdateTime) {
                return;
            }
        } else {
            if (order.UpdateTime >= data.UpdateTime) {
                return;
            }
        }

        order.OrderStatus = data.OrderStatus;
        order.ExecutePrice = data.ExecutePrice;
        order.ExecuteQuantity = data.ExecuteQuantity;
        order.UpdateTime = data.UpdateTime;

        this->transHandler(order);
    }
}