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
             WebsocketWrapper::BinanceOrderWrapper &orderWrapper,
            Pathfinder::Pathfinder &pathfinder,
            CapitalPool::CapitalPool &pool,
            HttpWrapper::BinanceApiWrapper &apiWrapper
    ) : ioService(ioService), orderWrapper(orderWrapper), TriangularArbitrage(pathfinder, pool, apiWrapper)
    {
        this->strategy = conf::MakerTriangular;
    }

    MakerTriangularArbitrage::~MakerTriangularArbitrage() = default;

    int MakerTriangularArbitrage::Run(string base, string quote) {
        double lockedQuantity;
        if (auto err = capitalPool.LockAsset("BUSD", 20, lockedQuantity); err > 0) {
            return err;
        }

        spdlog::info("{}::Run, symbol: {}", this->strategy.StrategyName, base+quote);

        this->OriginQuantity = lockedQuantity;
        this->TargetToken = "BUSD";
        this->currentPhase = 1;
        this->baseToken = base;
        this->quoteToken = quote;

        // todo 后续改成通用逻辑
        orderWrapper.SubscribeOrder(bind(&TriangularArbitrage::baseOrderHandler, this, std::placeholders::_1, std::placeholders::_2));

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

        if (data.Phase == 1)
        {
            return takerHandler(data);
        }

        if (data.Phase == 2)
        {
            return makerHandler(data);
        }

        if (data.Phase == 3)
        {
            this->currentPhase = this->currentPhase + 1;
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

        auto chance = pathfinder.FindBestPath(req);
        auto realProfit = data.GetParsePrice()*chance.Profit;
        if (realProfit > 1) {
            spdlog::info("!!!!path: {}", spdlog::fmt_lib::join(chance.Format(), ","));

            uint64_t orderId;
            auto err = ExecuteTrans(orderId, newPhase, chance.FirstStep());
            if (err > 0) {
                spdlog::error("{}::TransHandler, err: {}", this->strategy.StrategyName, WrapErr(err));
            }

            this->currentPhase = newPhase;
        } else {
            reorderTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService, websocketpp::lib::asio::milliseconds(20));
            reorderTimer->async_wait(bind(&MakerTriangularArbitrage::takerHandler, this, data));
        }
    }

    void MakerTriangularArbitrage::makerHandler(OrderData &data)
    {
        if (data.GetToToken() == this->TargetToken) {
            CheckFinish();
            return;
        }

        int newPhase = 3;

        uint64_t orderId;
        auto err = this->ReviseTrans(orderId, newPhase, data.GetToToken(), data.GetNewQuantity());
        if (err > 0)
        {
            spdlog::error("{}::TransHandler, err: {}", this->strategy.StrategyName, WrapErr(err));
            return;
        }
        this->currentPhase = newPhase;
    }

    int MakerTriangularArbitrage::partiallyFilledHandler(OrderData &orderData)
    {
        //部分转换之后放去ioc，剩余的部分放回资金池
        spdlog::info(
                "MakerTriangularArbitrage::partiallyFilledHandler::Now there are {} {} left",
                orderData.GetUnExecuteQuantity(), orderData.BaseToken
        );
        //如果剩余为稳定币 放回资金池 剩余订单取消
        if (define::IsStableCoin(orderData.GetFromToken()) || orderData.GetUnExecuteQuantity() > 0)
        {
            // 稳定币持仓，等待重平衡
            auto err = TriangularArbitrage::capitalPool.FreeAsset(orderData.GetFromToken(),
                                                                  orderData.GetUnExecuteQuantity());
            if (err > 0)
            {
                return err;
            }
            string symbol = apiWrapper.GetSymbol(orderData.BaseToken, orderData.QuoteToken);
            apiWrapper.CancelOrder(orderData.OrderId);
        }
        else if (!define::IsStableCoin(orderData.GetFromToken()) || orderData.GetUnExecuteQuantity() > 0)
        {
            uint64_t orderId;
            auto err = this->ReviseTrans(orderId, 0, orderData.GetFromToken(), orderData.GetUnExecuteQuantity());
            if (err > 0)
            {
                return err;
            }
        }
        //第一步至第二步
        if (orderData.GetToToken() != this->TargetToken)
        {
            uint64_t orderId;
            auto err = this->ReviseTrans(orderId, 0, orderData.GetToToken(), orderData.GetNewQuantity());
            if (err > 0)
            {
                return err;
            }
        }
        else if (orderData.GetToToken() == this->TargetToken)
        {
            FinalQuantity += orderData.GetNewQuantity();
            return 1;
        }
    }

    //价格变化幅度不够大，撤单重挂单
    //base是BUSD SIDE为BUY QUOTE放购入货币
    void MakerTriangularArbitrage::makerOrderChangeHandler(){
        if (this->PendingOrder != nullptr)
        {
            auto orderStatus = this->PendingOrder->OrderStatus;
            if (orderStatus == define::PARTIALLY_FILLED || orderStatus == define::FILLED)
            {
                return;
            }
        }

        reorderTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService, websocketpp::lib::asio::milliseconds(1 * 1000));
        reorderTimer->async_wait(bind(&MakerTriangularArbitrage::makerOrderChangeHandler, this));

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

        if (req.BaseToken == "BUSD")
        {
            newSide = define::SELL;
            newPrice = res.SellPrice * (1 + this->open);
            newQuantity = 20; // todo 固定20刀

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
            newQuantity = RoundDouble(20 / newPrice); // todo 固定20刀

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
            ExecuteTrans(orderId, 1, path);
            this->PendingOrder = orderMap[orderId];
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
                double cummulativeQuantity = 0;
                if (order->Side == define::SELL) {
                    cummulativeQuantity = RoundDouble(order->Price*order->Quantity);
                } else if (order->Side == define::BUY) {
                    cummulativeQuantity = order->Quantity;
                }

                auto data = OrderData{
                        .OrderId = orderId,
                        .OrderStatus = define::FILLED,
                        .ExecutePrice = order->Price,
                        .ExecuteQuantity = order->Quantity,
                        .CummulativeQuoteQuantity = cummulativeQuantity,
                        .UpdateTime = GetNowTime()
                };

                this->baseOrderHandler(data, 0);
            }
        }
    }
}