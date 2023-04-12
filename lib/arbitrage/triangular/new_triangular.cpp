#include "iostream"
#include <sstream>
#include "new_triangular.h"
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
                // spdlog::info("pathQuantity:{}", PathQuantity);
            }
            CheckFinish();
            return;
        }
    }

    void MakerTriangularArbitrage::takerHandler(OrderData &data)
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
                .OrderType = define::LIMIT
        };

        Pathfinder::GetExchangePriceResp resp{};
        if (auto err = this->pathfinder.GetExchangePrice(req, resp); err > 0)
        {
            spdlog::error("{}::TransHandler, err: {}", this->strategy.StrategyName, WrapErr(err));
            return;
        }
        int newPhase = 3;
        auto step = this->lastStep;
        step.OrderType = define::LIMIT;
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
        auto err = this->ExecuteTrans(orderId, newPhase, 0, step);
        if (err > 0)
        {
            spdlog::error("{}::TransHandler, err: {}", this->strategy.StrategyName, WrapErr(err));
            return;
        }
        this->currentPhase = newPhase;
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
            uint64_t cancelOrderId = 0;
            if (this->PendingOrder != nullptr) {
                cancelOrderId = this->PendingOrder->OrderId;
            }

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
            if (err = ExecuteTrans(orderId, 1, cancelOrderId, path); err == 0) {
                this->PendingOrder = orderMap[orderId];
            };
        }
    }
}