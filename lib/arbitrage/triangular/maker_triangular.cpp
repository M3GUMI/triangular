#include "iostream"
#include <sstream>
#include "maker_triangular.h"
#include "utils/utils.h"
#include "lib/pathfinder/graph.h"

using namespace boost::asio;
using namespace std;
namespace Arbitrage{
    MakerTriangularArbitrage::MakerTriangularArbitrage(
            websocketpp::lib::asio::io_service& ioService,
            Pathfinder::Pathfinder &pathfinder,
            CapitalPool::CapitalPool &pool,
            HttpWrapper::BinanceApiWrapper &apiWrapper
    ) : ioService(ioService), TriangularArbitrage(pathfinder, pool, apiWrapper)
    {
        this->strategy = conf::MakerTriangular;
        this->orderWrapper = new WebsocketWrapper::BinanceOrderWrapper(ioService, apiWrapper, "stream.binance.com", "9443");
        this->orderWrapper->SubscribeOrder(bind(&TriangularArbitrage::baseOrderHandler, this, std::placeholders::_1, std::placeholders::_2));

        pathfinder.SubscribeMock((bind(
                &MakerTriangularArbitrage::mockTrader,
                this,
                std::placeholders::_1,
                std::placeholders::_2,
                std::placeholders::_3,
                std::placeholders::_4
        )));
    }

    MakerTriangularArbitrage::~MakerTriangularArbitrage() = default;

    int MakerTriangularArbitrage::Run(const string& origin, const string& step, int amount) {
        double lockedQuantity;
        if (auto err = capitalPool.LockAsset(origin, amount, lockedQuantity); err > 0) {
            return err;
        }

        auto symbolData = apiWrapper.GetSymbolData(origin, step);
        spdlog::info("{}::Run, symbol: {}", this->strategy.StrategyName, symbolData.Symbol);

        this->OriginQuantity = lockedQuantity;
        this->TargetToken = origin;
        this->currentPhase = 1;
        this->baseToken = symbolData.BaseToken;
        this->quoteToken = symbolData.QuoteToken;

        MakerTriangularArbitrage::makerOrderChangeHandler();

        return 0;
    }

    void MakerTriangularArbitrage::TransHandler(OrderData &data)
    {
        spdlog::info(
                "{}::TransHandler, symbol: {}, side: {}, phase: {}, status: {}, quantity: {}, price: {}, executeQuantity: {}, newQuantity: {}",
                this->strategy.StrategyName,
                data.BaseToken+data.QuoteToken,
                sideToString(data.Side),
                data.Phase,
                data.OrderStatus,
                data.Quantity,
                data.Price,
                data.GetExecuteQuantity(),
                data.GetNewQuantity()
        );

        if (data.OrderStatus != define::FILLED)
        {
            // 暂定执行完才处理
            return;
        }

        if (data.GetToToken() == this->TargetToken) {
            FinalQuantity += data.GetNewQuantity();
        }
        if (data.Phase == 1 && data.OrderStatus == define::FILLED)
        {
            executeProfit = executeProfit * data.GetParsePrice();
            return takerHandler(data);
        }

        if (data.Phase == 2)
        {
            executeProfit = executeProfit * data.GetParsePrice();
            return makerHandler(data);
        }

        if (data.Phase == 3)
        {
            executeProfit = executeProfit * data.GetParsePrice();
            if (PathQuantity != 0){
                FinalQuantity += (PathQuantity-data.GetExecuteQuantity()) * data.Price;
                spdlog::info("pathQuantity:{}", PathQuantity);
            }
            spdlog::info("{}::Finish, profit: {}, originQuantity: {}, finialQuantity: {}",
                         this->strategy.StrategyName, FinalQuantity / OriginQuantity, OriginQuantity, FinalQuantity);
            CheckFinish();
            return;
        }
    }

    void MakerTriangularArbitrage::takerHandler(OrderData &data)
    {
        int newPhase = 2;
        // 寻找新路径重试
        Pathfinder::FindBestPathReq req{};
        req.Strategy = this->strategy;
        req.Origin = data.GetToToken();
        req.End = this->TargetToken;
        req.Quantity = data.GetNewQuantity();
        req.Phase = newPhase;

        //重试次数过多，终止
        if (retryTime > 10000 && !takerPathFinded){
            quitAndReopen = true;
        }

        if(!finished)
        {
            auto chance = pathfinder.FindBestPath(req);
            retryTime++;
            auto realProfit = data.GetParsePrice() * chance.Profit;
            if (realProfit > 1.0004)
            {
                takerPathFinded = true;
                uint64_t orderId;
                spdlog::info("{}::TakerHandler, realProfit: {}, best_path: {}", this->strategy.StrategyName, realProfit,
                             spdlog::fmt_lib::join(chance.Format(), ","));
                auto err = ExecuteTrans(orderId, newPhase, chance.FirstStep());
                if (err > 0)
                {
                    spdlog::error("{}::TakerHandler, err: {}", this->strategy.StrategyName, WrapErr(err));
                }

                this->lastStep = Pathfinder::TransactionPathItem{
                        .BaseToken = chance.Path[1].BaseToken,
                        .QuoteToken = chance.Path[1].QuoteToken,
                        .Side = chance.Path[1].Side,
                        .OrderType = chance.Path[1].OrderType,
                        .TimeInForce = chance.Path[1].TimeInForce,
                        .Price = chance.Path[1].Price,
                        .Quantity = chance.Path[1].Quantity,
                };

                this->currentPhase = newPhase;
            }
            else
            {
                if (quitAndReopen)
                {
                    spdlog::info("{}::TakerHandler,quitAndReopen：{}, can_not_find_path,quit_and_reopen_project",
                                 this->strategy.StrategyName, quitAndReopen);
                    CheckFinish();
                }
                // spdlog::info("{}::TakerHandler,failed_path:{}, profit:{}",this->strategy.StrategyName,  spdlog::fmt_lib::join(chance.Format(), ","),realProfit);
                retryTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService,
                                                                                    websocketpp::lib::asio::milliseconds(
                                                                                            20));
                retryTimer->async_wait(bind(&MakerTriangularArbitrage::takerHandler, this, data));
            }
        }
    }


    void MakerTriangularArbitrage::makerHandler(OrderData &data)
    {
        if (data.GetToToken() == this->TargetToken)
        {
            CheckFinish();
            return;
        }

        Pathfinder::GetExchangePriceReq req{
                .BaseToken = this->lastStep.BaseToken,
                .QuoteToken = this->lastStep.QuoteToken,
                .OrderType = define::LIMIT_MAKER
        };

        Pathfinder::GetExchangePriceResp resp{};
        if (auto err = this->pathfinder.GetExchangePrice(req, resp); err > 0)
        {
            spdlog::error("{}::TransHandler, err: {}", this->strategy.StrategyName, WrapErr(err));
            return;
        }
        int newPhase = 3;
        auto step = this->lastStep;
        if (step.Side == define::SELL)
        {
            if (resp.SellPrice > step.Price)
            {
                step.Price = resp.SellPrice;
            }
        }
        else
        {
            if (resp.BuyPrice < this->lastStep.Price)
            {
                step.Price = resp.BuyPrice;
            }
        }

        if (data.GetToToken() == this->lastStep.BaseToken)
        {
            step.Quantity = data.GetNewQuantity();
        }
        else if (data.GetToToken() == this->lastStep.QuoteToken)
        {
            step.Quantity = data.GetNewQuantity() / this->lastStep.Price;
        }

        uint64_t orderId;
        auto err = this->ExecuteTrans(orderId, newPhase, step);
        if (err > 0)
        {
            spdlog::error("{}::TransHandler, err: {}", this->strategy.StrategyName, WrapErr(err));
            return;
        }
        this->currentPhase = newPhase;

        reorderTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService, websocketpp::lib::asio::milliseconds(300*1000));
        reorderTimer->async_wait(bind(&MakerTriangularArbitrage::makerTimeoutHandler, this, orderId, data));
    }

    // maker挂单超时
    void MakerTriangularArbitrage::makerTimeoutHandler(uint64_t orderId, OrderData &data)
    {
        /*if (!orderMap.count(orderId)){
            return;
        }

        if (orderMap[orderId]->OrderStatus == define::FILLED){
            return;
        }

        apiWrapper.CancelOrder(orderId);*/
    }

    //价格变化幅度不够大，撤单重挂单
    //base是BUSD SIDE为BUY QUOTE放购入货币
    void MakerTriangularArbitrage::makerOrderChangeHandler(){
        reorderTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService, websocketpp::lib::asio::milliseconds(1 * 1000));
        reorderTimer->async_wait(bind(&MakerTriangularArbitrage::makerOrderChangeHandler, this));

        // socket重连暂时放在这里
        if (this->orderWrapper->Status == define::SocketStatusFailConnect) {
            this->orderWrapper = new WebsocketWrapper::BinanceOrderWrapper(ioService, apiWrapper, "stream.binance.com", "9443");
            this->orderWrapper->SubscribeOrder(bind(&TriangularArbitrage::baseOrderHandler, this, std::placeholders::_1, std::placeholders::_2));
        }

        if (this->PendingOrder != nullptr)
        {
            auto orderStatus = this->PendingOrder->OrderStatus;
            if (orderStatus == define::PARTIALLY_FILLED || orderStatus == define::FILLED)
            {
                return;
            }
        }

        Pathfinder::GetExchangePriceReq req;
        req.BaseToken = this->baseToken;
        req.QuoteToken = this->quoteToken;
        req.OrderType = define::LIMIT_MAKER;

        // 盘口价格
        Pathfinder::GetExchangePriceResp res;
        auto err = pathfinder.GetExchangePrice(req, res);
        if (err > 0) {
            spdlog::info("{}:makerOrderChangeHandler, err: {}", this->strategy.StrategyName, WrapErr(err));
            return;
        }

        bool needReOrder = false;
        define::OrderSide newSide = define::SELL;
        double newPrice = 0;
        double newQuantity = 0;

        if (req.BaseToken == this->TargetToken)
        {
            spdlog::info("nullptr");
            newSide = define::SELL;
            newPrice = res.SellPrice * (1 + this->open);
            newQuantity = this->OriginQuantity;

            if (this->PendingOrder != nullptr && res.SellPrice * (1 + this->close) < this->PendingOrder->Price)
            {
                // 盘口低于期望价格，撤单重挂
                needReOrder = true;
            }
        }
        else
        {
            newSide = define::BUY;
            newPrice = res.BuyPrice * (1 - this->open);
            newQuantity = RoundDouble(this->OriginQuantity / newPrice);

            if (this->PendingOrder != nullptr && res.BuyPrice * (1 - this->close) > this->PendingOrder->Price)
            {
                // 盘口高于期望价格，撤单重挂
                needReOrder = true;
            }
        }
        if (needReOrder || this->PendingOrder == nullptr) {
            // 取消旧单
            if (this->PendingOrder != nullptr) {
                apiWrapper.CancelOrder(this->PendingOrder->OrderId);
                this->PendingOrder = nullptr;
            } else {
                Pathfinder::TransactionPathItem path{
                        .BaseToken = this->baseToken,
                        .QuoteToken = this->quoteToken,
                        .Side = newSide,
                        .OrderType = define::LIMIT_MAKER,
                        .TimeInForce = define::GTC,
                        .Price = newPrice,
                        .Quantity = newQuantity
                };

                //挂出新单
                uint64_t orderId = 0;
                ExecuteTrans(orderId, 1, path);
                this->PendingOrder = orderMap[orderId];
            }
        }
    }

    void MakerTriangularArbitrage::mockTrader(const string& base, string quote, double buyPrice, double sellPrice) {
        if (!conf::EnableMock) {
            return;
        }

        for (auto& item:orderMap) {
            auto orderId = item.first;
            auto order = item.second;
            if (base != order->BaseToken || quote != order->QuoteToken) {
                continue;
            }

            if (order->OrderType != define::LIMIT_MAKER) {
                continue;
            }

            if (order->OrderStatus != define::INIT && order->OrderStatus != define::NEW) {
                continue;
            }

            bool execute = false;
            if (order->Side == define::SELL && order->Price < buyPrice) {
                execute = true;
            } else if (order->Side == define::BUY && order->Price > sellPrice) {
                execute = true;
            }

            if (execute) {
                auto data = OrderData{
                        .OrderId = orderId,
                        .Phase = order->Phase,
                        .OrderStatus = define::FILLED,
                        .ExecutePrice = order->Price,
                        .ExecuteQuantity = order->Quantity,
                        .CummulativeQuoteQuantity = RoundDouble(order->Price*order->Quantity),
                        .UpdateTime = GetNowTime()
                };

                this->baseOrderHandler(data, 0);
            }
        }
    }

    map<string, double> MakerTriangularArbitrage::mockPriceControl(OrderData& PendingOrder){
        double buyPrice = 0;
        double sellPrice = 0;
        if (PendingOrder.Side == define::SELL){
            sellPrice = PendingOrder.Price * 0.9993;
        }
        if (PendingOrder.Side == define::BUY){
            buyPrice = PendingOrder.Price * 1.0007;
        }
        mockTrader(PendingOrder.BaseToken, PendingOrder.QuoteToken, buyPrice, sellPrice);
//        map<string, double> prices;
//        int ram = rand() % 4 + 1;
//        switch (ram)
//        {
//            case 1:
//                prices["buyPrice"] = buyPrice = buyPrice * 1.05;
//                break;
//            case 2:
//                prices["buyPrice"] = buyPrice = buyPrice * 0.95;
//                break;
//            case 3:
//                prices["sellPrice"] = sellPrice = sellPrice * 1.05;
//                break;
//            case 4:
//                prices["sellPrice"] = sellPrice = sellPrice * 0.95;
//                break;
//        }
//        mockPriceTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService, websocketpp::lib::asio::milliseconds(30000));
//        mockPriceTimer->async_wait(bind(&MakerTriangularArbitrage::mockPriceControl, this, PendingOrder));
    }
}