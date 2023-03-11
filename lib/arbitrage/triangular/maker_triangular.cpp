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

        spdlog::info("MakerTriangularArbitrage::Run, symbol: {}", base+quote);

        this->TargetToken = "BUSD";
        this->OriginQuantity = lockedQuantity;
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
                "{}::TransHandler, base: {}, quote: {}, side: {}, status: {}, quantity: {}, price: {}, executeQuantity: {}, newQuantity: {}",
                this->strategy.StrategyName,
                data.BaseToken,
                data.QuoteToken,
                data.Side,
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

        if (data.Phase == 1 || data.Phase == 2)
        {
            // todo 需要加参数，改为市价taker单，非稳定币到稳定币
            // todo 需要加参数，改为稳定币到稳定币gtc挂单
            uint64_t orderId;
            auto err = this->ReviseTrans(orderId, data.Phase + 1, data.GetToToken(), data.GetExecuteQuantity());
            if (err > 0)
            {
                spdlog::error("{}::TransHandler, err: {}", this->strategy.StrategyName, WrapErr(err));
            }
            return;
        }

        if (data.Phase == 3)
        {
            CheckFinish();
            return;
        }
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
        reorderTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService, websocketpp::lib::asio::milliseconds(1 * 1000));
        reorderTimer->async_wait(bind(&MakerTriangularArbitrage::makerOrderChangeHandler, this));

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
}