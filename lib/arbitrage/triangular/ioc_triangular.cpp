#include "iostream"
#include "ioc_triangular.h"
#include "utils/utils.h"

namespace Arbitrage{
    IocTriangularArbitrage::IocTriangularArbitrage(
            Pathfinder::Pathfinder &pathfinder,
            CapitalPool::CapitalPool &pool,
            HttpWrapper::BinanceApiWrapper &apiWrapper
    ) : TriangularArbitrage(pathfinder, pool, apiWrapper) {
    }

    IocTriangularArbitrage::~IocTriangularArbitrage() = default;

    int IocTriangularArbitrage::Run(Pathfinder::ArbitrageChance &chance) {
        double lockedAmount = 0;
        auto firstStep = chance.FirstStep();
        if (auto err = capitalPool.LockAsset(firstStep.FromToken, firstStep.FromQuantity, lockedAmount); err > 0) {
            return err;
        }

        spdlog::info(
                "func: IocTriangularArbitrage::Run, profit: {}, path: {}",
                chance.Profit,
                spdlog::fmt_lib::join(chance.Format(), ","));

        firstStep.FromQuantity = lockedAmount;
        this->TargetToken = firstStep.FromToken;
        this->OriginQuantity = firstStep.FromQuantity;
        this->OriginToken = firstStep.FromToken;
        TriangularArbitrage::ExecuteTrans(firstStep);
        return 0;
    }

    void IocTriangularArbitrage::TransHandler(HttpWrapper::OrderData &data) {
        spdlog::info(
                "func: TransHandler, from: {}, to: {}, orderStatus: {}, executeQuantity: {}, newQuantity: {}",
                data.FromToken,
                data.ToToken,
                data.OrderStatus,
                data.ExecuteQuantity,
                data.ExecuteQuantity * data.ExecutePrice
        );

        int err;
        if (data.OrderStatus == define::FILLED) {
            err = filledHandler(data);
        } else if (data.OrderStatus == define::PARTIALLY_FILLED) {
            err = partiallyFilledHandler(data);
        } else {
            return;
        }

        if (err > 0) {
            spdlog::error("func: TransHandler, err: {}", WrapErr(err));
        }
    }

    int IocTriangularArbitrage::filledHandler(HttpWrapper::OrderData &data) {
        // 抵达目标
        if (data.ToToken != this->TargetToken) {
            return TriangularArbitrage::ReviseTrans(
                    data.ToToken, this->TargetToken, data.ExecuteQuantity * data.ExecutePrice
            );
        }

        if (CheckFinish()) {
            return Finish(data.ExecuteQuantity * data.ExecutePrice);
        }
        return define::ErrorStrategy;
    }

    int IocTriangularArbitrage::partiallyFilledHandler(HttpWrapper::OrderData &data) {
        // 处理未成交部分
        if (define::IsStableCoin(data.FromToken)) {
            // 稳定币持仓，等待重平衡
            TriangularArbitrage::capitalPool.FreeAsset(data.FromToken, data.OriginQuantity - data.ExecuteQuantity);
        } else if (define::NotStableCoin(data.FromToken)) {
            // 未成交部分重执行
            auto err = this->ReviseTrans(data.FromToken, this->TargetToken, data.OriginQuantity - data.ExecuteQuantity);
            if (err > 0) {
                return err;
            }
        }

        if (data.ToToken != this->TargetToken) {
            // 已成交部分继续执行
            auto err = this->ReviseTrans(data.ToToken, this->TargetToken, data.ExecuteQuantity * data.ExecutePrice);
            if (err > 0) {
                return err;
            }
        }
    }
}