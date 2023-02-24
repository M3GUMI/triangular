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
        string targetToken;
        double lockedQuantity;

        auto &firstStep = chance.FirstStep();
        if (firstStep.Side == define::SELL) {
            targetToken = firstStep.BaseToken;
        } else {
            targetToken = firstStep.QuoteToken;
        }

        if (auto err = capitalPool.LockAsset(targetToken, chance.Quantity, lockedQuantity); err > 0) {
            return err;
        }

        spdlog::info(
                "func: IocTriangularArbitrage::Run, profit: {}, quantity: {}, path: {}",
                chance.Profit,
                lockedQuantity,
                spdlog::fmt_lib::join(chance.Format(), ","));

        firstStep.Quantity = lockedQuantity;
        this->TargetToken = targetToken;
        this->OriginQuantity = lockedQuantity;
        TriangularArbitrage::ExecuteTrans(firstStep);
        return 0;
    }

    void IocTriangularArbitrage::TransHandler(HttpWrapper::OrderData &data) {
        spdlog::info(
                "func: TransHandler, base: {}, quote: {}, orderStatus: {}, quantity: {}, price: {}, executeQuantity: {}, newQuantity: {}",
                data.BaseToken,
                data.QuoteToken,
                data.OrderStatus,
                data.Quantity,
                data.Price,
                data.ExecuteQuantity,
                data.ExecuteQuantity * data.Price
        );

        // 完全失败, 终止
        if (data.GetFromToken() == TargetToken && data.ExecuteQuantity == 0) {
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
            Finish(data.ExecuteQuantity * data.Price);
            return;
        }
    }

    int IocTriangularArbitrage::filledHandler(HttpWrapper::OrderData &data) {
        // 抵达目标
        if (data.GetToToken() == this->TargetToken) {
            return 0;
        }

        return TriangularArbitrage::ReviseTrans(
                data.GetToToken(), this->TargetToken, data.ExecuteQuantity * data.Price
        );
    }

    int IocTriangularArbitrage::partiallyFilledHandler(HttpWrapper::OrderData &data) {
        // 处理未成交部分
        if (define::IsStableCoin(data.GetFromToken())) {
            // 稳定币持仓，等待重平衡
            auto err = TriangularArbitrage::capitalPool.FreeAsset(data.GetFromToken(), data.Quantity - data.ExecuteQuantity);
            if (err > 0) {
                return err;
            }
        } else if (define::NotStableCoin(data.GetFromToken())) {
            // 未成交部分重执行
            auto err = this->ReviseTrans(data.GetFromToken(), this->TargetToken, data.Quantity - data.ExecuteQuantity);
            if (err > 0) {
                return err;
            }
        }

        if (data.GetToToken() != this->TargetToken) {
            // 已成交部分继续执行
            auto err = this->ReviseTrans(data.GetToToken(), this->TargetToken, data.CummulativeQuoteQuantity);
            if (err > 0) {
                return err;
            }
        }
    }
}