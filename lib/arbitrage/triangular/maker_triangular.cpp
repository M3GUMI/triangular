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
        this->strategyV2 = conf::MakerTriangular;
    }

    MakerTriangularArbitrage::~MakerTriangularArbitrage() {
    }

    int MakerTriangularArbitrage::Run(Pathfinder::ArbitrageChance &chance) {
        double lockedQuantity;
        auto &firstStep = chance.FirstStep();
        string fromToken = firstStep.GetFromToken();
        if (auto err = capitalPool.LockAsset(fromToken, chance.Quantity, lockedQuantity); err > 0) {
            return err;
        }

        if (firstStep.OrderType != define::LIMIT_MAKER) {
            return ErrorInvalidParam;
        }

        spdlog::info(
                "MakerTriangularArbitrage::Run, profit: {}, quantity: {}, path: {}",
                chance.Profit,
                lockedQuantity,
                spdlog::fmt_lib::join(chance.Format(), ","));

        firstStep.Quantity = lockedQuantity;
        this->TargetToken = fromToken;
        this->OriginQuantity = lockedQuantity;

        // todo 后续改成通用逻辑
        orderWrapper.SubscribeOrder(bind(&TriangularArbitrage::baseOrderHandler, this, std::placeholders::_1, std::placeholders::_2));

        uint64_t orderId;
        TriangularArbitrage::ExecuteTrans(orderId, 1, firstStep);

        reorderTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService, websocketpp::lib::asio::milliseconds(5 * 1000));
        reorderTimer->async_wait(bind(&MakerTriangularArbitrage::makerOrderChangeHandler, this));
        return 0;
    }

    void MakerTriangularArbitrage::TransHandler(OrderData &data)
    {
        spdlog::info(
                "{}::TransHandler, base: {}, quote: {}, side: {}, status: {}, quantity: {}, price: {}, executeQuantity: {}, newQuantity: {}",
                this->strategyV2.StrategyName,
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
                spdlog::error("{}::TransHandler, err: {}", this->strategyV2.StrategyName, WrapErr(err));
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
        reorderTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService, websocketpp::lib::asio::milliseconds(5 * 1000));
        reorderTimer->async_wait(bind(&MakerTriangularArbitrage::makerOrderChangeHandler, this));

        auto baseToken = this->PendingOrder->BaseToken;
        auto quoteToken = this->PendingOrder->QuoteToken;
        auto orderStatus = this->PendingOrder->OrderStatus;
        auto price = this->PendingOrder->Price;

        if (orderStatus == define::PARTIALLY_FILLED || orderStatus == define::FILLED)
        {
            return;
        }

        Pathfinder::GetExchangePriceReq req;
        req.BaseToken = baseToken;
        req.QuoteToken = quoteToken;
        req.OrderType = this->PendingOrder->OrderType;

        // 盘口价格
        Pathfinder::GetExchangePriceResp res;
        pathfinder.GetExchangePrice(req, res);

        bool needReOrder = false;
        define::OrderSide newSide = define::SELL;
        double newPrice = 0;
        double newQuantity = 0;

        if (req.BaseToken == "BUSD") {
            if (res.SellPrice*(1+this->close) < price) {
                // 盘口低于期望价格，撤单重挂
                needReOrder = true;
                newSide = define::SELL;
                newPrice = res.SellPrice*(1+this->open);
                newQuantity = 20; // todo 固定20刀
            }
        } else {
            if (res.BuyPrice*(1-this->close) > price) {
                // 盘口高于期望价格，撤单重挂
                needReOrder = true;
                newSide = define::BUY;
                newPrice = res.BuyPrice*(1-this->open);
                newQuantity = RoundDouble(20 / newPrice); // todo 固定20刀
            }
        }

        if (needReOrder) {
            // 取消旧单
            apiWrapper.CancelOrder(this->PendingOrder->OrderId);

            Pathfinder::TransactionPathItem path{
                    .BaseToken = baseToken,
                    .QuoteToken = quoteToken,
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