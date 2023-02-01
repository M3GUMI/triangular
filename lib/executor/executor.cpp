#include <functional>
#include "executor.h"
#include "lib/arbitrage/triangular/triangular.h"
#include "lib/pathfinder/pathfinder.h"
#include "lib/arbitrage/triangular/triangular.h"

using namespace std;
namespace Executor
{
	Executor::Executor(Pathfinder::Pathfinder &pathfinder) : pathfinder(pathfinder)
	{
		this->pathfinder.SubscribeArbitrage(bind(&Executor::arbitrageDataHandler, this, placeholders::_1));
		return;
	}

	Executor::~Executor()
	{
	}

	void Executor::arbitrageDataHandler(Pathfinder::TransactionPath& path)
	{
		Arbitrage::TriangularArbitrage triangular(this->pathfinder);
		triangular.Run(path);
	}
}