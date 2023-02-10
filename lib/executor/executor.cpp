#include <functional>
#include "utils/utils.h"
#include "lib/arbitrage/triangular/ioc_triangular.h"
#include "executor.h"

using namespace std;
namespace Executor
{
	Executor::Executor(Pathfinder::Pathfinder &pathfinder, CapitalPool::CapitalPool &capitalPool, HttpWrapper::BinanceApiWrapper &apiWrapper) : pathfinder(pathfinder), capitalPool(capitalPool), apiWrapper(apiWrapper)
	{
		pathfinder.SubscribeArbitrage((bind(&Executor::arbitragePathHandler, this, placeholders::_1)));
	}

	Executor::~Executor()
	{
	}

	void Executor::arbitragePathHandler(Pathfinder::TransactionPath &path)
	{
		if (lock)
		{
            spdlog::debug("func: {}, msg: {}", "arbitragePathHandler", "arbitrage executing, ignore path");
			return;
		}

		Arbitrage::IocTriangularArbitrage iocTriangular(pathfinder, capitalPool, apiWrapper);
		// todo 需要增加套利任务结束，清除subscribe
		iocTriangular.SubscribeFinish(bind(&Executor::arbitrageFinishHandler, this));
		if (auto err = iocTriangular.Run(path); err > 0) {
			return;
		}

		lock = true;
	}

	void Executor::arbitrageFinishHandler()
	{
		lock = false;
	}
}