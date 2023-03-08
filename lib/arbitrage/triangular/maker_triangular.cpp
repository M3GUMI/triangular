#include "iostream"
#include <sstream>
#include "maker_triangular.h"
#include "utils/utils.h"
#include "conf/strategy.h"
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

        TriangularArbitrage::ExecuteTrans(1, firstStep);

        reorderTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService, websocketpp::lib::asio::milliseconds(5 * 1000));
        reorderTimer->async_wait(bind(&MakerTriangularArbitrage::makerOrderChangeHandler, this, firstStep));
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
            auto err = this->ReviseTrans(data.GetToToken(), this->TargetToken, data.Phase + 1,
                                         data.GetExecuteQuantity());
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
            auto err = this->ReviseTrans(orderData.GetFromToken(), this->TargetToken, 0, orderData.GetUnExecuteQuantity());
            if (err > 0)
            {
                return err;
            }
        }
        //第一步至第二步
        if (orderData.GetToToken() != this->TargetToken)
        {
            auto err = this->ReviseTrans(orderData.GetToToken(), this->TargetToken, 0, orderData.GetNewQuantity());
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

    /*
     * maker操作处理器
     * */
    //价格变化幅度不够大，撤单重挂单
    //base是BUSD SIDE为BUY QUOTE放购入货币
    void MakerTriangularArbitrage::makerOrderChangeHandler(Pathfinder::TransactionPathItem &lastPath){
        Pathfinder::GetExchangePriceReq req;
        string QuoteToken = apiWrapper.GetSymbolData(lastPath.BaseToken, lastPath.QuoteToken).QuoteToken;
        string  BaseToken = apiWrapper.GetSymbolData(lastPath.BaseToken, lastPath.QuoteToken).BaseToken;
        req.QuoteToken = QuoteToken;
        req.BaseToken = BaseToken;
        Pathfinder::GetExchangePriceResp res;
        pathfinder.GetExchangePrice(req, res);
        double nowPrice = res.BuyPrice;
        double nowQuantity = res.BuyQuantity;
        define::TimeInForce TimeInForce = define::GTC;
        double lastPrice = lastPath.Price;
        double priceSpread = nowPrice - lastPrice;
        double newUpPrice, newDownPrice;
        //判断是否为第一次发出订单
        if(lastPath.Price == 0)
        {
            if ((priceSpread < 0 && threshold >= -priceSpread) || (priceSpread > 0 && threshold >= priceSpread))
            {
                newUpPrice = nowPrice + threshold;
                newDownPrice = nowPrice + threshold;
            }
            else
            {
                //超过阈值但未交易
                spdlog::info("MakerTriangularArbitrage::  order{}->{} can be finish but not,lockcedPrice = {},threshold ={}, buyPrice = {}", lastPath.QuoteToken,
                             lastPath.BaseToken, lastPath.Price, threshold, res.BuyPrice);
            }
        }
        else
        {
            newUpPrice = nowPrice + threshold;
            newDownPrice = nowPrice + threshold;
        }
        Pathfinder::TransactionPathItem path;

        path.BaseToken = lastPath.BaseToken;
        path.QuoteToken = lastPath.QuoteToken;
        path.TimeInForce = TimeInForce;
        path.Price = newDownPrice;
        path.Quantity = nowQuantity;
        path.OrderType = define::LIMIT_MAKER;
        path.Side = define::BUY;
        //构建新订单完毕
        //挂出新单
        ExecuteTrans(0, path);
    };
}