#pragma once
#include "triangular.h"

using namespace std;
namespace Arbitrage {
    // 两次maker挂单三角套利
    class MakerTriangularArbitrage : public TriangularArbitrage {
    public:
        MakerTriangularArbitrage(Pathfinder::Pathfinder &pathfinder, CapitalPool::CapitalPool &pool,
                                 HttpWrapper::BinanceApiWrapper &apiWrapper);

        ~MakerTriangularArbitrage();

        int Run(Pathfinder::ArbitrageChance &chance);
        void makerOrderChangeHandler(OrderData &lastOrder);//价格变化幅度不够大，撤单重挂单
        int makerOrderIocHandler(OrderData &orderData);//挂单交易成功，后续ioc操作
    private:
        void TransHandler(double threshold,OrderData &depthData,OrderData &orderData);
        double threshold;
        int FilledHandler(OrderData &orderData);
        int partiallyFilledHandler(OrderData &orderData);
} ;
}