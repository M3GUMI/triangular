#pragma once
#include <string>
#include <time.h>
#include "lib/arbitrage/triangular/triangular.h"

namespace Executor
{
	class Executor
	{
	private:
		Pathfinder::Pathfinder &pathfinder;
		void arbitrageDataHandler(Pathfinder::TransactionPath &path);

	public:
		Executor(Pathfinder::Pathfinder &pathfinder);
	};
}
