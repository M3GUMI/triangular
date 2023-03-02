#pragma once
#include "triangular.h"

using namespace std;
namespace Arbitrage{
    // 两次ioc三角套利
    class IocTriangularArbitrage : public TriangularArbitrage {
    public:
        IocTriangularArbitrage(
                Pathfinder::Pathfinder &pathfinder,
                CapitalPool::CapitalPool &pool,
                HttpWrapper::BinanceApiWrapper &apiWrapper
        );
        ~IocTriangularArbitrage();

        int Run(Pathfinder::ArbitrageChance &chance) override;

    private:
        void TransHandler(OrderData &orderData) override;
        int filledHandler(OrderData &data);
        int partiallyFilledHandler(OrderData &data);
        int expiredHandler(OrderData &data);
    };
}