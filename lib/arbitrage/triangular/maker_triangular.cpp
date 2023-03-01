#include "iostream"
#include <sstream>
#include "maker_triangular.h"
#include "utils/utils.h"

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

    int MakerTriangularArbitrage::Run(double threshold,OrderData &newOrder) {
        OrderData order = {};
        orderMap[newOrder.OrderId] = &newOrder;
        makerOrderChangeHandler(threshold, newOrder, order);
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
            cancelOrder(orderData);
            //逻辑上应该有问题
        makerOrderIocHandler(orderData);
    }

        return makerOrderIocHandler(orderData);
}
}