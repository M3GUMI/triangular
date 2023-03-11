#include "triangular.h"
#include "lib/api/http/binance_api_wrapper.h"



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

    int TriangularArbitrage::ExecuteTrans(uint64_t& orderId, int phase, Pathfinder::TransactionPathItem &path) {
        auto order = new OrderData();
        order->OrderId = GenerateId();
        order->Phase = phase;
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
        orderId = order->OrderId;

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

    int TriangularArbitrage::ReviseTrans(uint64_t& orderId, int phase, string origin, double quantity) {
        // 寻找新路径重试
        Pathfinder::FindBestPathReq req{};
        req.Strategy = this->strategyV2;
        req.Origin = origin;
        req.End = this->TargetToken;
        req.Quantity = quantity;
        req.Phase = phase;

        auto chance = pathfinder.FindBestPath(req);
        spdlog::info(
                "{}::RevisePath, profit: {}, quantity: {}, bestPath: {}",
                this->strategy,
                quantity*chance.Profit/this->OriginQuantity,
                chance.FirstStep().Quantity,
                spdlog::fmt_lib::join(chance.Format(), ","));

        return ExecuteTrans(orderId, phase, chance.FirstStep());
    }

    void TriangularArbitrage::baseOrderHandler(OrderData &data, int err) {
        if (err > 0) {
            spdlog::error("func: baseOrderHandler, err: {}", WrapErr(err));
            return;
        }

        if (not orderMap.count(data.OrderId)) {
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


    //执行挂单操作
    int TriangularArbitrage::executeOrder(OrderData& orderData)
    {
            auto order = new OrderData();
            order->OrderId = GenerateId();
            order->BaseToken = orderData.BaseToken;
            order->QuoteToken = orderData.QuoteToken;
            order->Side = orderData.Side;
            order->Price = orderData.Price;
            order->Quantity = orderData.Quantity;
            order->OrderType = orderData.OrderType;
            order->TimeInForce = orderData.TimeInForce;
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
        double lockedQuantity;
        if (auto err = capitalPool.LockAsset(orderData.BaseToken, orderData.Quantity, lockedQuantity); err > 0) {
            return err;
        }
            spdlog::info(
                    "func: ExecuteTrans, err: {}, base: {}, quote: {}, side: {}, price: {}, quantity: {}",
                    err,
                    orderData.BaseToken,
                    orderData.QuoteToken,
                    sideToString(orderData.Side),
                    orderData.Price,
                    orderData.Quantity
            );

            if (err == define::ErrorLessTicketSize || err == define::ErrorLessMinNotional) {
                return 0;
            }

            return err;
    }

    int TriangularArbitrage::cancelOrder(OrderData &orderData){
        string token0 = orderData.BaseToken;
        string token1 = orderData.QuoteToken;
        string symbol = apiWrapper.GetSymbol(token0, token1);
//        define::OrderSide = apiWrapper.GetSide(token0, token1);
        apiWrapper.CancelOrder(orderData.OrderId);
    }

}