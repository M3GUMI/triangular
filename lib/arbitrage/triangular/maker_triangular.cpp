#include "iostream"
#include <sstream>
#include "maker_triangular.h"
#include "utils/utils.h"
#include "lib/pathfinder/graph.h"
#include <boost/asio.hpp>


using namespace boost::asio;
namespace Arbitrage{
    MakerTriangularArbitrage::MakerTriangularArbitrage(
            Pathfinder::Pathfinder &pathfinder,
            CapitalPool::CapitalPool &pool,
            HttpWrapper::BinanceApiWrapper &apiWrapper
    ) : TriangularArbitrage(pathfinder, pool, apiWrapper) {
        this->strategy = "maker";
    }

    MakerTriangularArbitrage::~MakerTriangularArbitrage() {
    }

    int MakerTriangularArbitrage::Run(Pathfinder::ArbitrageChance &chance) {
        auto &firstStep = chance.FirstStep();
        double lockedQuantity;
        string fromToken;
        if (firstStep.Side == define::BUY) {
            fromToken = firstStep.BaseToken;
        } else {
            spdlog::info(
                    "MakerTriangularArbitrage::side Error,side can only be {}",
                    define::BUY);
            return 0;
        }
        spdlog::info(
                "MakerTriangularArbitrage::Run, profit: {}, quantity: {}, path: {}",
                chance.Profit,
                lockedQuantity,
                spdlog::fmt_lib::join(chance.Format(), ","));

        if (auto err = capitalPool.LockAsset(fromToken, chance.Quantity, lockedQuantity); err > 0) {
            return err;
        }
        firstStep.Quantity = lockedQuantity;
        this->TargetToken = fromToken;
        this->OriginQuantity = lockedQuantity;
        TriangularArbitrage::ExecuteTrans(firstStep);
        io_service ioService;
        reorderTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService, websocketpp::lib::asio::milliseconds(5 * 1000));
        reorderTimer->async_wait(bind(&MakerTriangularArbitrage::makerOrderChangeHandler, this, firstStep));
        return 0;
    }

    void MakerTriangularArbitrage::TransHandler(OrderData &data) {
        spdlog::info(
                "makerTriangularArbitrage::Handler, base: {}, quote: {}, orderStatus: {}, quantity: {}, price: {}, executeQuantity: {}, newQuantity: {}",
                data.BaseToken,
                data.QuoteToken,
                data.OrderStatus,
                data.Quantity,
                data.Price,
                data.GetExecuteQuantity(),
                data.GetNewQuantity()
        );
        //完全交易
        int err = 0;
        if (data.OrderStatus == define::FILLED){
            MakerTriangularArbitrage::FilledHandler(data);
        }
        else if (data.OrderStatus == define::PARTIALLY_FILLED){
            MakerTriangularArbitrage::partiallyFilledHandler(data);
        }
    }

    int MakerTriangularArbitrage::FilledHandler(OrderData &orderData){
        if (orderData.GetToToken() == this->TargetToken) {
            FinalQuantity += orderData.GetNewQuantity();
            return 0;
        }
        //全部转换之后放去进行ioc
        spdlog::info(
                " MakerTriangularArbitrage::FilledHandler::Now there are {} {} left"
                ,orderData.GetUnExecuteQuantity()
                ,orderData.BaseToken
                );
        return makerOrderIocHandler(orderData);
    }


    int MakerTriangularArbitrage::partiallyFilledHandler(OrderData &orderData){
        //部分转换之后放去ioc，剩余的部分放回资金池
        spdlog::info(
                "MakerTriangularArbitrage::partiallyFilledHandler::Now there are {} {} left"
                ,orderData.GetUnExecuteQuantity()
                ,orderData.BaseToken
        );
        //如果剩余为稳定币 放回资金池 剩余订单取消
        if (define::IsStableCoin(orderData.GetFromToken()) || orderData.GetUnExecuteQuantity() > 0 ) {
            // 稳定币持仓，等待重平衡
            auto err = TriangularArbitrage::capitalPool.FreeAsset(orderData.GetFromToken(), orderData.GetUnExecuteQuantity());
            if (err > 0) {
                return err;
            }
            string symbol = apiWrapper.GetSymbol(orderData.BaseToken, orderData.QuoteToken);
            apiWrapper.CancelOrder(orderData.OrderId);
        }
        else if(!define::IsStableCoin(orderData.GetFromToken()) || orderData.GetUnExecuteQuantity() > 0){
            auto err = this->ReviseTrans(orderData.GetFromToken(), this->TargetToken, orderData.GetUnExecuteQuantity());
            if (err > 0) {
                return err;
            }
        }
        //第一步至第二步
        if (orderData.GetToToken() != this->TargetToken){
            auto err = this->ReviseTrans(orderData.GetToToken(), this->TargetToken, orderData.GetNewQuantity());
            if (err > 0) {
                return err;
            }
        }
        else if(orderData.GetToToken() == this->TargetToken)
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
        else{
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
        ExecuteTrans( path );
    };

    //挂单交易成功，后续ioc操作
    int MakerTriangularArbitrage::makerOrderIocHandler(OrderData &orderData){
        return ReviseTrans(orderData.QuoteToken, orderData.BaseToken, orderData.GetNewQuantity());
    };
}