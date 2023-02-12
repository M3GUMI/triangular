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
        Pathfinder::TransactionPathItem firstPath = chance.Path.front();
        if (auto err = capitalPool.LockAsset(firstPath.FromToken, firstPath.FromQuantity); err > 0) {
            return err;
        }

        this->TargetToken = firstPath.FromToken;
        this->OriginQuantity = firstPath.FromQuantity;
        this->OriginToken = firstPath.FromToken;
        AddBalance(firstPath.FromToken, firstPath.FromQuantity);
        TriangularArbitrage::ExecuteTrans(firstPath);
        return 0;
    }

    void IocTriangularArbitrage::TransHandler(HttpWrapper::OrderData &data) {
        spdlog::info(
                "func: {}, OrderStatus: {}, ExecuteQuantity: {}, OriginQuantity: {}",
                "TransHandler",
                data.OrderStatus,
                data.ExecuteQuantity,
                data.OriginQuantity
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
            spdlog::error("func: {}, err: {}", "orderDataHandler", WrapErr(err));
        }
    }

    int IocTriangularArbitrage::filledHandler(HttpWrapper::OrderData &data) {
        if (CheckFinish()) {
            return Finish(data.ExecuteQuantity * data.ExecutePrice);
        }

        return TriangularArbitrage::ReviseTrans(
                data.ToToken, this->TargetToken, data.ExecuteQuantity * data.ExecutePrice
        );
    }

    int IocTriangularArbitrage::partiallyFilledHandler(HttpWrapper::OrderData &data) {
        // 处理未成交部分
        if (define::IsStableCoin(data.FromToken)) {
            // 稳定币持仓，等待重平衡
            TriangularArbitrage::capitalPool.FreeAsset(data.FromToken, data.ExecuteQuantity);
            DelBalance(data.FromToken, data.ExecuteQuantity);
        } else if (define::NotStableCoin(data.FromToken)) {
            // 未成交部分重执行
            auto err = this->ReviseTrans(data.FromToken, this->TargetToken, data.OriginQuantity - data.ExecuteQuantity);
            if (err > 0) {
                return err;
            }
        }

        // 已成交部分继续执行
        auto err = this->ReviseTrans(data.ToToken, this->TargetToken, data.ExecuteQuantity * data.ExecutePrice);
        if (err > 0) {
            return err;
        }
    }
}