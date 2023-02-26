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

    int MakerTriangularArbitrage::Run(Pathfinder::ArbitrageChance &chance) {
        /*spdlog::info("func: {}, msg: {}", "Run", "MakerTriangularArbitrage start");
        Pathfinder::TransactionPathItem firstPath = chance.Path[0];
        double lockAmount = 0;
        if (auto err = capitalPool.LockAsset(firstPath.FromToken, firstPath.FromQuantity, lockAmount); err > 0) {
            return err;
        }

        this->OriginQuantity = firstPath.FromQuantity;
        this->OriginToken = firstPath.FromToken;
        this->TargetToken = firstPath.FromToken;
        TriangularArbitrage::ExecuteTrans(firstPath);*/
        return 0;
    }

    void MakerTriangularArbitrage::TransHandler(OrderData &data) {
        // todo 需要增加套利任务的资金池锁定金额。需要增加订单管理
        // 1. 每一步maker盘口挂单
        // 2. 实时监测盘口价格，超过阈值则取消挂单重新挂
        // 3. 检测maker单成交数量，记录在套利任务的资金池中
        // 4. 资金池中某笔数量达到一定阈值。调用算法层，继续执行maker挂单
    }
}