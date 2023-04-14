#include "iostream"
#include <sstream>
#include "new_triangular.h"
#include "utils/utils.h"
#include "lib/pathfinder/graph.h"

using namespace boost::asio;
using namespace std;
namespace Arbitrage{
    NewTriangularArbitrage::NewTriangularArbitrage(
            websocketpp::lib::asio::io_service& ioService,
            Pathfinder::Pathfinder &pathfinder,
            CapitalPool::CapitalPool &pool,
            HttpWrapper::BinanceApiWrapper &apiWrapper
    ) : ioService(ioService), TriangularArbitrage(pathfinder, pool, apiWrapper)
    {
        this->strategy = conf::MakerTriangular;
        this->orderWrapper = new WebsocketWrapper::BinanceOrderWrapper(ioService, apiWrapper, "stream.binance.com", "9443");
        this->orderWrapper->SubscribeOrder(bind(&TriangularArbitrage::baseOrderHandler, this, std::placeholders::_1, std::placeholders::_2));
        this->pathfinder.SubscribeMock(bind(&NewTriangularArbitrage::makerOrderChangeHandler, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

        pathfinder.SubscribeMock((bind(
                &NewTriangularArbitrage::mockTrader,
                this,
                std::placeholders::_1,
                std::placeholders::_2,
                std::placeholders::_3,
                std::placeholders::_4
        )));
    }

    NewTriangularArbitrage::~NewTriangularArbitrage() = default;

    int NewTriangularArbitrage::Run(const string& origin, const string& step, int amount) {
        double lockedQuantity;
        if (auto err = capitalPool.LockAsset(origin, amount, lockedQuantity); err > 0) {
            return err;
        }

        auto symbolData = apiWrapper.GetSymbolData(origin, step);
        spdlog::info("{}::Run, symbol: {}", this->strategy.StrategyName, symbolData.Symbol);

        this->OriginQuantity = lockedQuantity;
        this->TargetToken = origin;
        this->currentPhase = 1;

        return 0;
    }

    void NewTriangularArbitrage::TransHandler(OrderData &data)
    {
        if (finished) {
            return;
        }

        spdlog::info(
                "{}::TransHandler, orderId: {}, symbol: {}, side: {}, phase: {}, status: {}, quantity: {}, price: {}, executeQuantity: {}, newQuantity: {}",
                this->strategy.StrategyName,
                data.OrderId,
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
        if (data.Phase == 1)
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
                // spdlog::info("pathQuantity:{}", PathQuantity);
            }
            CheckFinish();
            return;
        }
    }

    void NewTriangularArbitrage::takerHandler(OrderData &data)
    {
        // 第二步，立刻taker出eth
        int newPhase = 2;
        auto symbolData = apiWrapper.GetSymbolData("ETH", data.GetToToken());
        Pathfinder::GetExchangePriceReq req{
                .BaseToken = symbolData.BaseToken,
                .QuoteToken = symbolData.QuoteToken,
                .OrderType = define::MARKET
        };

        Pathfinder::GetExchangePriceResp resp{};
        if (auto err = this->pathfinder.GetExchangePrice(req, resp); err > 0)
        {
            spdlog::error("{}::TransHandler, err: {}", this->strategy.StrategyName, WrapErr(err));
            return;
        }

        Pathfinder::TransactionPathItem path{
                .BaseToken = symbolData.BaseToken,
                .QuoteToken = symbolData.QuoteToken,
                .Side = apiWrapper.GetSide(data.GetToToken(), "ETH"),
                .OrderType = define::MARKET,
                .TimeInForce = define::IOC,
                .Price = data.GetToToken() == symbolData.BaseToken? resp.SellPrice: resp.BuyPrice,
                .Quantity = data.GetNewQuantity(),
        };

        uint64_t orderId;
        auto err = ExecuteTrans(orderId, newPhase, 0, path);
        if (err > 0)
        {
            spdlog::error("{}::TakerHandler, err: {}", this->strategy.StrategyName, WrapErr(err));
        }

        this->currentPhase = newPhase;
    }

    void NewTriangularArbitrage::makerHandler(OrderData &data)
    {
        if (data.GetToToken() == this->TargetToken)
        {
            CheckFinish();
            return;
        }

        // 第三步，立刻挂单为usdt
        int newPhase = 3;
        auto symbolData = apiWrapper.GetSymbolData("USDT", data.GetToToken());
        Pathfinder::GetExchangePriceReq req{
                .BaseToken = symbolData.BaseToken,
                .QuoteToken = symbolData.QuoteToken,
                .OrderType = define::MARKET
        };

        Pathfinder::GetExchangePriceResp resp{};
        if (auto err = this->pathfinder.GetExchangePrice(req, resp); err > 0)
        {
            spdlog::error("{}::TransHandler, err: {}", this->strategy.StrategyName, WrapErr(err));
            return;
        }

        Pathfinder::TransactionPathItem path{
                .BaseToken = symbolData.BaseToken,
                .QuoteToken = symbolData.QuoteToken,
                .Side = apiWrapper.GetSide(data.GetToToken(), "USDT"),
                .OrderType = define::LIMIT_MAKER,
                .TimeInForce = define::GTC,
                .Price = data.GetToToken() == symbolData.BaseToken? resp.SellPrice: resp.BuyPrice,
                .Quantity = data.GetNewQuantity(),
        };

        uint64_t orderId;
        auto err = this->ExecuteTrans(orderId, newPhase, 0, path);
        if (err > 0)
        {
            spdlog::error("{}::TransHandler, err: {}", this->strategy.StrategyName, WrapErr(err));
            return;
        }
        this->currentPhase = newPhase;
    }

    void NewTriangularArbitrage::socketReconnectHandler() {
        if (this->orderWrapper->Status == define::SocketStatusFailConnect) {
            this->orderWrapper = new WebsocketWrapper::BinanceOrderWrapper(ioService, apiWrapper, "stream.binance.com", "9443");
            this->orderWrapper->SubscribeOrder(bind(&TriangularArbitrage::baseOrderHandler, this, std::placeholders::_1, std::placeholders::_2));
        }
        socketReconnectTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService, websocketpp::lib::asio::milliseconds(1 * 1000));
        socketReconnectTimer->async_wait(bind(&NewTriangularArbitrage::socketReconnectHandler, this));
    }
    //价格变化幅度不够大，撤单重挂单
    //base是BUSD SIDE为BUY QUOTE放购入货币
    void NewTriangularArbitrage::makerOrderChangeHandler(const string& base, const string& quote, double buyPrice, double sellPrice){
        spdlog::info("makerOrderChangeHandler base:{}, quote:{}, buyPrice:{}, sellPrice:{}", base, quote, buyPrice, sellPrice);

        // 非第一步
        if (base != "XRP" || quote != "USDT") {
            return;
        }

        if (this->currentPhase != 1) {
            return;
        }

        if (this->PendingOrder != nullptr)
        {
            auto orderStatus = this->PendingOrder->OrderStatus;
            if (orderStatus == define::PARTIALLY_FILLED || orderStatus == define::FILLED)
            {
                return;
            }
        }

        auto needReOrder = false;
        auto newSide = define::BUY;
        auto newPrice = buyPrice * (1 - this->open);
        auto newQuantity = RoundDouble(this->OriginQuantity / newPrice);

        if (this->PendingOrder != nullptr && buyPrice * (1 - this->close) > this->PendingOrder->Price)
        {
            // 盘口高于期望价格，撤单重挂
            needReOrder = true;
        }

        if (needReOrder || this->PendingOrder == nullptr) {
            uint64_t cancelOrderId = 0;
            if (this->PendingOrder != nullptr) {
                cancelOrderId = this->PendingOrder->OrderId;
            }

            Pathfinder::TransactionPathItem path{
                    .BaseToken = base,
                    .QuoteToken = quote,
                    .Side = newSide,
                    .OrderType = define::LIMIT_MAKER,
                    .TimeInForce = define::GTC,
                    .Price = newPrice,
                    .Quantity = newQuantity
            };

            //挂出新单
            uint64_t orderId = 0;
            int err;
            if (err = ExecuteTrans(orderId, 1, cancelOrderId, path); err == 0) {
                this->PendingOrder = orderMap[orderId];
            };
        }
    }

    void NewTriangularArbitrage::mockTrader(const string& base, string quote, double buyPrice, double sellPrice) {
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
}