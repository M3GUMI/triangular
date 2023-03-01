#include "iostream"
#include <sstream>
#include "maker_triangular.h"
#include "utils/utils.h"
#include "pathfinder/graph.h"

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
        string fromToken;
        if (firstStep.Side == define::BUY) {
            fromToken = firstStep.BaseToken;
        } else {
            fromToken = firstStep.QuoteToken;
        }
        OrderData order = {};
//        orderMap[newOrder.OrderId] = &newOrder;
        makerOrderChangeHandler(order);
        return 0;
    }

    void MakerTriangularArbitrage::TransHandler(double threshold,OrderData &depthData,OrderData &data) {
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
        //全部转换之后放去进行ioc
        spdlog::info(
                "Now there are {} {} left"
                ,orderData.GetUnExecuteQuantity()
                ,orderData.BaseToken
                );
        return makerOrderIocHandler(orderData);
    }


    int MakerTriangularArbitrage::partiallyFilledHandler(OrderData &orderData){
        //部分转换之后放去ioc，剩余的部分放回资金池
        spdlog::info(
                "Now there are {} {} left"
                ,orderData.GetUnExecuteQuantity()
                ,orderData.BaseToken
        );
        if (define::IsStableCoin(orderData.GetFromToken())) {
            // 稳定币持仓，等待重平衡
            auto err = TriangularArbitrage::capitalPool.FreeAsset(orderData.GetFromToken(), orderData.GetUnExecuteQuantity());
            if (err > 0) {
                return err;
            }
            string symbol = apiWrapper.GetSymbol(orderData.BaseToken, orderData.QuoteToken);

            //逻辑上应该有问题
        makerOrderIocHandler(orderData);
    }

        return makerOrderIocHandler(orderData);
}

    /*
     * maker操作处理器
     * */
    //价格变化幅度不够大，撤单重挂单
    //base是BUSD SIDE为BUY QUOTE放购入货币
    void MakerTriangularArbitrage::makerOrderChangeHandler(OrderData &lastOrder){
        Pathfinder::GetExchangePriceReq req;
        string QuoteToken = apiWrapper.GetSymbolData(lastOrder.BaseToken, lastOrder.QuoteToken).QuoteToken;
        string  BaseToken = apiWrapper.GetSymbolData(lastOrder.BaseToken, lastOrder.QuoteToken).BaseToken;
        req.QuoteToken = QuoteToken;
        req.BaseToken = BaseToken;
        Pathfinder::GetExchangePriceResp res;
        Pathfinder::Graph::GetExchangePrice(req, res);
        double nowPrice = res.BuyPrice;
        double nowQuantity = res.BuyQuantity;
        define::TimeInForce TimeInForce = define::GTC;
        double lastPrice = lastOrder.Price;
        double priceSpread = nowPrice - lastPrice;
        double newUpPrice, newDownPrice;
        //判断是否为第一次发出订单
        if(lastOrder.Price == 0)
        {
            if ((priceSpread < 0 && threshold >= -priceSpread) || (priceSpread > 0 && threshold >= priceSpread))
            {
                newUpPrice = nowPrice + threshold;
                newDownPrice = nowPrice + threshold;
            }
            else
            {
                //超过阈值但未交易
                spdlog::info("MakerTriangularArbitrage::  order{}->{} can be finish but not,orderId = {}", lastOrder.QuoteToken,
                             lastOrder.BaseToken, lastOrder.OrderId);
            }
        }
        else{
            newUpPrice = nowPrice + threshold;
            newDownPrice = nowPrice + threshold;
        }
        OrderData *newMakerOrder;
        newMakerOrder->BaseToken = lastOrder.BaseToken;
        newMakerOrder->QuoteToken = lastOrder.QuoteToken;
        newMakerOrder->TimeInForce = TimeInForce;
        newMakerOrder->BaseToken = lastOrder.BaseToken;
        newMakerOrder->Price = newDownPrice;
        newMakerOrder->Quantity = nowQuantity;
        newMakerOrder->OrderStatus = define::INIT;
        newMakerOrder->UpdateTime = GetNowTime();
        //构建新订单完毕
        //挂出新单
        executeOrder( *newMakerOrder );
    };

    //挂单交易成功，后续ioc操作
    int MakerTriangularArbitrage::makerOrderIocHandler(OrderData &orderData){
        return ReviseTrans(orderData.QuoteToken, orderData.BaseToken, orderData.GetNewQuantity());
    };
}