#include <functional>
#include "lib/arbitrage/triangular/triangular.h"
#include "lib/pathfinder/pathfinder.h"
#include "lib/arbitrage/triangular/triangular.h"
#include "utils/utils.h"
#include "executor.h"

using namespace std;
namespace Executor
{
	Executor::Executor(Pathfinder::Pathfinder &pathfinder) : pathfinder(pathfinder)
	{
		this->pathfinder.SubscribeArbitrage(bind(&Executor::arbitragePathHandler, this, placeholders::_1));
		return;
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

		Arbitrage::TriangularArbitrage triangular(this->pathfinder);
		triangular.SubscribeFinish(bind(&Executor::arbitrageFinishHandler, this));
		triangular.Run(path);
		lock = true;
	}

	void Executor::arbitrageFinishHandler()
	{
		lock = false;
	}
}