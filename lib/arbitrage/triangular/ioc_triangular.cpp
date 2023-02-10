#include "iostream"
#include <sstream>
#include "ioc_triangular.h"
#include "utils/utils.h"

namespace Arbitrage {
    IocTriangularArbitrage::IocTriangularArbitrage(Pathfinder::Pathfinder &pathfinder, CapitalPool::CapitalPool &pool,
                                                   HttpWrapper::BinanceApiWrapper &apiWrapper) : TriangularArbitrage(
            pathfinder, pool, apiWrapper) {
        this->transHandler = bind(&IocTriangularArbitrage::TransHandler, this, placeholders::_1);
    }

    IocTriangularArbitrage::~IocTriangularArbitrage() {
    }

    int IocTriangularArbitrage::Run(Pathfinder::TransactionPath &path) {
        spdlog::info("func: {}, msg: {}", "Run", "IocTriangularArbitrage start");
        Pathfinder::TransactionPathItem firstPath = path.Path.front();
        if (auto err = capitalPool.LockAsset(firstPath.FromToken, firstPath.FromQuantity); err > 0) {
            return err;
        }

        this->OriginQuantity = firstPath.FromQuantity;
        this->OriginToken = firstPath.FromToken;
        this->TargetToken = firstPath.FromToken;
        TriangularArbitrage::ExecuteTrans(firstPath);
        return 0;
    }

    void IocTriangularArbitrage::TransHandler(HttpWrapper::OrderData &data) {
        spdlog::info("func: {}, OrderStatus: {}, ExecuteQuantity: {}, OriginQuantity: {}", "TransHandler", data.OrderStatus, data.ExecuteQuantity, data.OriginQuantity);
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
        // todo 判断逻辑有误，需要判断是否所有从属订单执行完成
        if (data.ToToken == this->TargetToken) {
            // 套利完成
            return Finish(data.ExecuteQuantity * data.ExecutePrice);
        }

        return TriangularArbitrage::ReviseTrans(
                data.ToToken, this->TargetToken, data.ExecuteQuantity * data.ExecutePrice);
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

        // 已成交部分继续执行
        auto err = this->ReviseTrans(data.ToToken, this->TargetToken, data.ExecuteQuantity * data.ExecutePrice);
        if (err > 0) {
            return err;
        }
    }
}