#pragma once
#include <string>
#include <time.h>
#include "lib/arbitrage/triangular/triangular.h"

namespace Executor
{
	class Executor
	{
	private:
		bool lock; // 同时只执行一个套利任务

		void arbitragePathHandler(Pathfinder::TransactionPath &path);
		void arbitrageFinishHandler();

	public:
		Executor();
		~Executor();
	};
}
