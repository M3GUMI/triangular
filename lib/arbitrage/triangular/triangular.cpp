#include "triangular.h"

namespace Arbitrage{
    TriangularArbitrage::TriangularArbitrage(
            Pathfinder::Pathfinder &pathfinder,
            CapitalPool::CapitalPool &pool,
            HttpWrapper::BinanceApiWrapper &apiWrapper
    ) : pathfinder(pathfinder), capitalPool(pool), apiWrapper(apiWrapper) {
    }

    TriangularArbitrage::~TriangularArbitrage() = default;


    int TriangularArbitrage::Run(Pathfinder::ArbitrageChance &chance)
    {
        exit(EXIT_FAILURE);
    }

    bool TriangularArbitrage::CheckFinish()
    {
        if (finished) {
            return true;
        }

        for (const auto& item : orderMap)
        {
            auto order = item.second;
            if (order->OrderStatus != define::FILLED &&
                order->OrderStatus != define::PARTIALLY_FILLED &&
                order->OrderStatus != define::EXPIRED)
            {
                return false;
            }
        }

        spdlog::info("{}::Finish, profit: {}, finalQuantity: {}, originQuantity: {}",
                     this->strategy, this->FinalQuantity / this->OriginQuantity, this->FinalQuantity, this->OriginQuantity);
        finished = true;
        this->subscriber();
        return true;
    }

    void TriangularArbitrage::SubscribeFinish(function<void()> callback) {
        this->subscriber = callback;
    }

    int TriangularArbitrage::ExecuteTrans(Pathfinder::TransactionPathItem &path) {
        auto order = new OrderData();
        order->OrderId = GenerateId();
        order->BaseToken = path.BaseToken;
        order->QuoteToken = path.QuoteToken;
        order->Side = path.Side;
        order->Price = path.Price;
        order->Quantity = path.Quantity;
        order->OrderType = path.OrderType;
        order->TimeInForce = path.TimeInForce;
        order->OrderStatus = define::INIT;
        order->UpdateTime = GetNowTime();
        orderMap[order->OrderId] = order;

        auto err = apiWrapper.CreateOrder(
                *order,
                bind(
                        &TriangularArbitrage::baseOrderHandler,
                        this,
                        placeholders::_1,
                        placeholders::_2
                ));
        spdlog::info(
                "{}::ExecuteTrans, err: {}, base: {}, quote: {}, side: {}, orderType: {}, price: {}, calPrice: {}, quantity: {}",
                this->strategy,
                err,
                path.BaseToken,
                path.QuoteToken,
                sideToString(path.Side),
                orderTypeToString(path.OrderType),
                path.Price,
                1/path.Price,
                path.Quantity
        );

        if (err == define::ErrorLessTicketSize || err == define::ErrorLessMinNotional) {
            return 0;
        }

        return err;
    }

    int TriangularArbitrage::ReviseTrans(string origin, string end, double quantity) {
        // 寻找新路径重试
        auto chance = pathfinder.FindBestPath(this->strategy, origin, end, quantity);
        spdlog::info(
                "{}::RevisePath, profit: {}, quantity: {}, bestPath: {}",
                this->strategy,
                quantity*chance.Profit/this->OriginQuantity,
                chance.FirstStep().Quantity,
                spdlog::fmt_lib::join(chance.Format(), ","));

        return ExecuteTrans(chance.FirstStep());
    }

    void TriangularArbitrage::baseOrderHandler(OrderData &data, int err) {
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

        OrderData* order = orderMap[data.OrderId];
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
        order->CummulativeQuoteQuantity = data.CummulativeQuoteQuantity;
        order->UpdateTime = data.UpdateTime;

        TransHandler(*order);
        TriangularArbitrage::CheckFinish();
    }

    void TriangularArbitrage::TransHandler(OrderData &orderData) {
        exit(EXIT_FAILURE);
    }
}