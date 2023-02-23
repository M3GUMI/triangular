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
        auto &firstStep = chance.FirstStep();
        if (auto err = capitalPool.LockAsset(firstStep.FromToken, firstStep.FromQuantity, lockedAmount); err > 0) {
            return err;
        }

        spdlog::info(
                "func: IocTriangularArbitrage::Run, profit: {}, quantity: {}, path: {}",
                chance.Profit,
                lockedAmount,
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
                "func: TransHandler, from: {}, to: {}, orderStatus: {}, originQuantity: {}, executePrice: {}, executeQuantity: {}, newQuantity: {}",
                data.FromToken,
                data.ToToken,
                data.OrderStatus,
                data.OriginQuantity,
                data.ExecutePrice,
                data.ExecuteQuantity,
                data.ExecuteQuantity * data.ExecutePrice
        );

        // 完全失败, 终止
        if (data.FromToken == OriginToken && data.ExecuteQuantity == 0) {
            TriangularArbitrage::Finish(0);
            return;
        }

        int err = 0;
        if (data.OrderStatus == define::FILLED || data.OrderStatus == define::EXPIRED) {
            err = filledHandler(data);
        } else if (data.OrderStatus == define::PARTIALLY_FILLED) {
            err = partiallyFilledHandler(data);
        } else {
            return;
        }

        if (err > 0) {
            spdlog::error("func: TransHandler, err: {}", err);
            return;
        }

        if (CheckFinish()) {
            Finish(data.ExecuteQuantity * data.ExecutePrice);
            return;
        }
    }

    int IocTriangularArbitrage::filledHandler(HttpWrapper::OrderData &data) {
        // 抵达目标
        if (data.ToToken == this->TargetToken) {
            return 0;
        }

        return TriangularArbitrage::ReviseTrans(
                data.ToToken, this->TargetToken, data.ExecuteQuantity * data.ExecutePrice
        );
    }

    int IocTriangularArbitrage::partiallyFilledHandler(HttpWrapper::OrderData &data) {
        // 处理未成交部分
        if (define::IsStableCoin(data.FromToken)) {
            // 稳定币持仓，等待重平衡
            auto err = TriangularArbitrage::capitalPool.FreeAsset(data.FromToken, data.OriginQuantity - data.ExecuteQuantity);
            if (err > 0) {
                return err;
            }
        } else if (define::NotStableCoin(data.FromToken)) {
            // 未成交部分重执行
            auto err = this->ReviseTrans(data.FromToken, this->TargetToken, data.OriginQuantity - data.ExecuteQuantity);
            if (err > 0) {
                return err;
            }
        }

        if (data.ToToken != this->TargetToken) {
            // 已成交部分继续执行
            auto err = this->ReviseTrans(data.ToToken, this->TargetToken, data.CummulativeQuoteQuantity);
            if (err > 0) {
                return err;
            }
        }
    }
}