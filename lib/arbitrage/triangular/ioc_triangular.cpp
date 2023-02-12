#include "iostream"
#include <sstream>
#include "ioc_triangular.h"
#include "utils/utils.h"

namespace Arbitrage{
    IocTriangularArbitrage::IocTriangularArbitrage(
            Pathfinder::Pathfinder &pathfinder,
            CapitalPool::CapitalPool &pool,
            HttpWrapper::BinanceApiWrapper &apiWrapper
    ) : TriangularArbitrage(pathfinder, pool, apiWrapper) {
        this->transHandler = bind(&IocTriangularArbitrage::TransHandler, this, placeholders::_1);
    }

    IocTriangularArbitrage::~IocTriangularArbitrage() {
    }

    int IocTriangularArbitrage::Run(Pathfinder::ArbitrageChance &chance) {
        Pathfinder::TransactionPathItem firstPath = chance.Path.front();
        double lockAmount = 0;
        if (auto err = capitalPool.LockAsset(firstPath.FromToken, firstPath.FromQuantity, lockAmount); err > 0) {
            return err;
        }
        firstPath.FromQuantity = lockAmount;

        vector<string> info;
        info.push_back(chance.Path.front().FromToken);
        for (const auto &item: chance.Path) {
            info.push_back(to_string(item.FromPrice));
            info.push_back(item.ToToken);
        }

        spdlog::info(
                "func: IocTriangularArbitrage::Run, profit: {}, path: {}",
                chance.Profit, spdlog::fmt_lib::join(info, ",")
        );

        this->TargetToken = firstPath.FromToken;
        this->OriginQuantity = firstPath.FromQuantity;
        this->OriginToken = firstPath.FromToken;
        TriangularArbitrage::ExecuteTrans(firstPath);
        return 0;
    }

    void IocTriangularArbitrage::TransHandler(HttpWrapper::OrderData &data) {
        spdlog::info(
                "func: TransHandler, From: {}, To: {}, OrderStatus: {}, ExecutePrice: {}, ExecuteQuantity: {}, OriginQuantity: {}",
                data.FromToken, data.ToToken, data.OrderStatus, data.ExecutePrice, data.ExecuteQuantity, data.OriginQuantity
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
    }

    int IocTriangularArbitrage::partiallyFilledHandler(HttpWrapper::OrderData &data) {
        // 处理未成交部分
        if (define::IsStableCoin(data.FromToken)) {
            // 稳定币持仓，等待重平衡
            TriangularArbitrage::capitalPool.FreeAsset(data.FromToken, data.ExecuteQuantity);
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