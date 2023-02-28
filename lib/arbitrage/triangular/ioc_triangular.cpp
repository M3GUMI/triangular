#include "iostream"
#include "ioc_triangular.h"
#include "utils/utils.h"

namespace Arbitrage{
    IocTriangularArbitrage::IocTriangularArbitrage(
            Pathfinder::Pathfinder &pathfinder,
            CapitalPool::CapitalPool &pool,
            HttpWrapper::BinanceApiWrapper &apiWrapper
    ) : TriangularArbitrage(pathfinder, pool, apiWrapper) {
        this->strategy = "taker";
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
                "IocTriangularArbitrage::Run, profit: {}, quantity: {}, path: {}",
                chance.Profit,
                lockedQuantity,
                spdlog::fmt_lib::join(chance.Format(), ","));


        if (firstStep.Side == define::BUY) {
            lockedQuantity /= firstStep.Price;
        }

        firstStep.Quantity = lockedQuantity;
        this->TargetToken = targetToken;
        this->OriginQuantity = lockedQuantity;
        TriangularArbitrage::ExecuteTrans(firstStep);
        return 0;
    }

    void IocTriangularArbitrage::TransHandler(OrderData &data) {
        spdlog::info(
                "IocTriangularArbitrage::Handler, base: {}, quote: {}, orderStatus: {}, quantity: {}, price: {}, executeQuantity: {}, newQuantity: {}",
                data.BaseToken,
                data.QuoteToken,
                data.OrderStatus,
                data.Quantity,
                data.Price,
                data.GetExecuteQuantity(),
                data.GetNewQuantity()
        );

        // 完全失败, 终止
        if (data.GetFromToken() == TargetToken && data.GetExecuteQuantity() == 0) {
            if(TriangularArbitrage::CheckFinish()) {
                spdlog::info("!!!!!!!!!!!!finish");
                return;
            }
        }

        int err = 0;
        if (data.OrderStatus == define::FILLED) {
            err = filledHandler(data);
        } else if (data.OrderStatus == define::PARTIALLY_FILLED) {
            err = partiallyFilledHandler(data);
        } else {
            return;
        }

        if (err > 0) {
            spdlog::error("func: TransHandler, err: {}", WrapErr(err));
            return;
        }
    }

    int IocTriangularArbitrage::filledHandler(OrderData &data) {
        // 抵达目标
        if (data.GetToToken() == this->TargetToken) {
            FinalQuantity += data.GetNewQuantity();
            return 0;
        }

        return TriangularArbitrage::ReviseTrans(
                data.GetToToken(), this->TargetToken,data.GetNewQuantity()
        );
    }

    int IocTriangularArbitrage::partiallyFilledHandler(OrderData &data) {
        // 处理未成交部分
        /*if (define::IsStableCoin(data.GetFromToken())) {
            // 稳定币持仓，等待重平衡
            auto err = TriangularArbitrage::capitalPool.FreeAsset(data.GetFromToken(), data.GetUnExecuteQuantity());
            if (err > 0) {
                return err;
            }
        } else if (define::NotStableCoin(data.GetFromToken())) {*/
            // 未成交部分重执行
            auto err = this->ReviseTrans(data.GetFromToken(), this->TargetToken, data.GetUnExecuteQuantity());
            if (err > 0) {
                return err;
            }
        //}

        if (data.GetToToken() != this->TargetToken) {
            // 已成交部分继续执行
            auto err = this->ReviseTrans(data.GetToToken(), this->TargetToken, data.GetNewQuantity());
            if (err > 0) {
                return err;
            }
        } else {
            FinalQuantity += data.GetNewQuantity();
        }
    }
}