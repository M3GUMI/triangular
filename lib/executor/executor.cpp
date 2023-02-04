#include <functional>
#include "utils/utils.h"
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

	void Executor::arbitragePathHandler(Pathfinder::TransactionPath& path)
	{
		if (lock) {
			LogInfo("func", "arbitragePathHandler", "msg", "arbitrage executing, ignore path");
			return;
		}

		Arbitrage::TriangularArbitrage triangular(pathfinder, capitalPool, apiWrapper);
		triangular.SubscribeFinish(bind(&Executor::arbitrageFinishHandler, this));
		triangular.Run(path);
		lock = true;
	}

	void Executor::arbitrageFinishHandler()
	{
		lock = false;
	}
}