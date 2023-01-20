#include <functional>
#include "executor.h"
#include "defined/defined.h"
#include "lib/arbitrage/triangular/triangular.h"
#include "lib/pathfinder/pathfinder.h"
#include "lib/arbitrage/triangular/triangular.h"

using namespace std;
namespace Executor
{
	Executor::Executor()
	{
		// 回调订阅
		Pathfinder::Pathfinder pathfinder; 
		this->pathfinder = &pathfinder;
		this->pathfinder->SubscribeArbitrage(bind(&Executor::arbitrageDataHandler, this, placeholders::_1));

		return;
	}

	void Executor::arbitrageDataHandler(Pathfinder::TransactionPath *path)
	{
		// 当前不区分类型，均为三角套利

		Arbitrage::TriangularArbitrage triangular;
		// 1. 新建三角套利任务,注册任务并初始化执行
		triangular.Run(this->pathfinder, path);
	}
}