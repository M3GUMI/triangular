#include <functional>
#include "utils/utils.h"
#include "lib/arbitrage/triangular/ioc_triangular.h"
#include "executor.h"

using namespace std;
namespace Executor{
    Executor::Executor(
            Pathfinder::Pathfinder &pathfinder,
            CapitalPool::CapitalPool &capitalPool,
            HttpWrapper::BinanceApiWrapper &apiWrapper
    ) : pathfinder(pathfinder), capitalPool(capitalPool), apiWrapper(apiWrapper) {
        pathfinder.SubscribeArbitrage((bind(&Executor::arbitragePathHandler, this, placeholders::_1)));
    }

    Executor::~Executor() {
    }

    void Executor::arbitragePathHandler(Pathfinder::ArbitrageChance &chance) {
        if (this->lock) {
            spdlog::info("lock");
            return;
        }

        // todo 此处需要内存管理。需要增加套利任务结束，清除subscribe
        this->lock = true;
        Arbitrage::TriangularArbitrage *iocTriangular = new Arbitrage::IocTriangularArbitrage(
                pathfinder,
                capitalPool,
                apiWrapper
        );
        iocTriangular->SubscribeFinish(bind(&Executor::arbitrageFinishHandler, this));
        if (auto err = iocTriangular->Run(chance); err > 0) {
            this->lock = false;
            return;
        }
    }

    void Executor::arbitrageFinishHandler() {
        this->lock = false;
        capitalPool.Refresh();
    }
}