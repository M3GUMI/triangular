#include "triangular.h"
#include "utils/utils.h"

namespace Arbitrage {
    TriangularArbitrage::TriangularArbitrage(Pathfinder::Pathfinder &pathfinder, CapitalPool::CapitalPool &pool,
                                             HttpWrapper::BinanceApiWrapper &apiWrapper) : pathfinder(pathfinder),
                                                                                           capitalPool(pool),
                                                                                           apiWrapper(apiWrapper) {
    }

    TriangularArbitrage::~TriangularArbitrage() {
    }

    int TriangularArbitrage::Finish(double finalQuantity) {
        LogInfo("func", "Finish", "finalQuantity", to_string(finalQuantity), "originQuantity",
                to_string(this->OriginQuantity));
        capitalPool.Refresh();
        this->subscriber();
        return 0;
    }

    void TriangularArbitrage::SubscribeFinish(function<void()> callback) {
        this->subscriber = callback;
    }

    int TriangularArbitrage::ExecuteTrans(Pathfinder::TransactionPathItem &path) {
        // todo 需要新增套利任务结束时释放
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

        LogInfo("func", "ExecuteTrans", "from", path.FromToken, "to", path.ToToken);
        LogInfo("price", to_string(path.FromPrice), "quantity", to_string(path.FromQuantity));
        return apiWrapper.CreateOrder(req, bind(&TriangularArbitrage::baseOrderHandler, this, placeholders::_1,
                                                placeholders::_2));
    }

    int TriangularArbitrage::ReviseTrans(string origin, string end, double quantity) {
        // 寻找新路径重试
        Pathfinder::RevisePathReq req;
        Pathfinder::TransactionPath resp;

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
            LogError("func", "baseOrderHandler", "err", WrapErr(err));
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

        this->transHandler(order);
    }
}