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
        spdlog::info("triangular Run");
        exit(EXIT_FAILURE);
    }

    bool TriangularArbitrage::CheckFinish()
    {
        if (finished) {
            spdlog::info("finished:{}", finished);
            return true;
        }
        if(quitAndReopen){
            finished = true;
            if (this->subscriber != nullptr) {
                this->subscriber();
            }
            return true;
        }
        for (const auto& item : orderMap)
        {
            auto order = item.second;
            if (order->Phase == 1) {
                // 现不判断1阶段单
                if (order->OrderStatus != define::FILLED &&
                    order->OrderStatus != define::PARTIALLY_FILLED &&
                    order->OrderStatus != define::EXPIRED &&
                    order->OrderStatus != define::NEW ) {
                    spdlog::info("orderId: {}, phase: {}, symbol:{}",
                                 order->OrderId, order->Phase, order->BaseToken+order->QuoteToken);
                }
                continue;
            }
            if (order->OrderStatus != define::FILLED &&
                order->OrderStatus != define::PARTIALLY_FILLED &&
                order->OrderStatus != define::EXPIRED &&
                order->OrderStatus != define::NEW )
            {
                spdlog::info("orderId: {}, orderStatus:{}, phase: {}, symbol:{}, side:{}, ExecuteQuantity:{}, NewQuantity:{}, finished:false",
                             order->OrderId, order->OrderStatus, order->Phase, order->BaseToken+order->QuoteToken, order->Side, order->ExecuteQuantity,order->GetNewQuantity());
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

    int TriangularArbitrage::ExecuteTrans(uint64_t& orderId, int phase, uint64_t cancelOrderId, Pathfinder::TransactionPathItem &path) {
        auto order = new OrderData();
        order->OrderId = GenerateId();
        order->CancelOrderId = cancelOrderId;
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
        if(path.Quantity >= symbolData.StepSize)
        {
            uint32_t tmpQuantity = path.Quantity / symbolData.StepSize;
            order->Quantity = tmpQuantity * symbolData.StepSize;
        }
        if (path.Quantity < symbolData.StepSize){
            order->Quantity = path.Quantity;
        }

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

        return ExecuteTrans(orderId, phase, 0, chance.FirstStep());
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
        if (!orderStatusCheck(order->OrderStatus, data.OrderStatus)) {
            return;
        }

        if (data.Quantity > 0) {
            order->Quantity = data.Quantity;
        }

        order->OrderStatus = data.OrderStatus;
        order->ExecutePrice = data.ExecutePrice;
        order->ExecuteQuantity = data.ExecuteQuantity;
        order->CummulativeQuoteQuantity = data.CummulativeQuoteQuantity;
        order->UpdateTime = data.UpdateTime;

        if(order->OrderStatus == define::FILLED){
            if(order->Phase == 1){
                OriginQuantity = order->GetExecuteQuantity();
            }
            else if(order->Phase == 2 ){
                PathQuantity = order->GetNewQuantity();
            }
            else if(data.Phase == 3 ){
                MakerExecuted = true;
            }
        }

        TransHandler(*order);
        // TriangularArbitrage::CheckFinish();
    }

    bool TriangularArbitrage::orderStatusCheck(int baseStatus, int newStatus){
        // 状态机
        // INIT->NEW
        // NEW->PARTIALLY_FILLED->FILLED
        // NEW->EXPIRED
        if (baseStatus == define::INIT) {
            return true;
        }
        if (newStatus == define::CANCELED || newStatus == define::REJECTED || newStatus == define::EXPIRED) {
            return true;
        }

        if (baseStatus == define::NEW && newStatus == define::PARTIALLY_FILLED) {
            return true;
        }
        if (baseStatus == define::PARTIALLY_FILLED && newStatus == define::FILLED) {
            return true;
        }
        if (baseStatus == define::NEW && newStatus == define::FILLED) {
            return true;
        }

        return false;
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