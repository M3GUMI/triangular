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
                     this->strategy.StrategyName, this->FinalQuantity / this->OriginQuantity, this->FinalQuantity, this->OriginQuantity);
        finished = true;
        if (this->subscriber != nullptr) {
            this->subscriber();
        }
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
        order->OrderType = path.OrderType;
        order->TimeInForce = path.TimeInForce;
        order->OrderStatus = define::INIT;
        order->UpdateTime = GetNowTime();

        // ticketSize校验
        auto symbolData = apiWrapper.GetSymbolData(path.BaseToken, path.QuoteToken);
        uint32_t tmpPrice = path.Price / symbolData.TicketSize;
        order->Price = tmpPrice * symbolData.TicketSize;

        // stepSize校验
        uint32_t tmpQuantity = path.Quantity / symbolData.StepSize;
        order->Quantity = tmpQuantity * symbolData.StepSize;

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
                "{}::ExecuteTrans, phase:{}, symbol: {}, side: {}, orderType: {}, price: {}, quantity: {}",
                this->strategy.StrategyName,
                order->Phase,
                order->BaseToken+order->QuoteToken,
                sideToString(order->Side),
                orderTypeToString(order->OrderType),
                order->Price,
                order->Quantity
        );
        if (err > 0) {
            spdlog::info(
                    "{}::ExecuteTrans, err: {}",
                    this->strategy.StrategyName,
                    WrapErr(err)
            );
        }

        if (err == define::ErrorLessTicketSize || err == define::ErrorLessMinNotional) {
            return 0;
        }

        return err;
    }

    int TriangularArbitrage::ReviseTrans(uint64_t& orderId, int phase, string origin, double quantity) {
        // 寻找新路径重试
        Pathfinder::FindBestPathReq req{};
        req.Strategy = this->strategy;
        req.Origin = origin;
        req.End = this->TargetToken;
        req.Quantity = quantity;
        req.Phase = phase;

        auto chance = pathfinder.FindBestPath(req);
//        spdlog::info(
//                "{}::RevisePath, profit: {}, quantity: {}, bestPath: {}",
//                this->strategy.StrategyName,
//                quantity*chance.Profit/this->OriginQuantity,
//                chance.FirstStep().Quantity,
//                spdlog::fmt_lib::join(chance.Format(), ","));

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

        if (data.Quantity > 0) {
            order->Quantity = data.Quantity;
        }

        order->OrderStatus = data.OrderStatus;
        order->ExecutePrice = data.ExecutePrice;
        order->ExecuteQuantity = data.ExecuteQuantity;
        order->CummulativeQuoteQuantity = data.CummulativeQuoteQuantity;
        order->UpdateTime = data.UpdateTime;

        //堵住重复调用
        if (order->Phase == 1 && overFirstStep == true && order->OrderStatus == define::FILLED){
            spdlog::info("overFirstStep = true, status:{}", order->OrderStatus );
            return;
        }

        if(order->Phase == 1 && overFirstStep == false && order->OrderStatus == define::FILLED ){
            overFirstStep = true;
                OriginQuantity = order->GetExecuteQuantity();
        }
        else if(order->Phase == 2 ){
              PathQuantity = order->GetNewQuantity();
        }
        else if(data.Phase == 3 ){
               }

        TransHandler(*order);
        // TriangularArbitrage::CheckFinish();
    }

    void TriangularArbitrage::TransHandler(OrderData &orderData) {
        exit(EXIT_FAILURE);
    }

    map<uint64_t, OrderData*> TriangularArbitrage::getOrderMap(){
        return orderMap;
    }
    void TriangularArbitrage::setOrderMap(map<uint64_t, OrderData*> newOrderMap){
        orderMap = newOrderMap;
    }

}