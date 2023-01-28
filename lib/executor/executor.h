#pragma once
#include <string>
#include <time.h>
#include "lib/arbitrage/triangular/triangular.h"

using namespace std;
namespace Executor
{
	struct NotifyOrderUpdateReq
	{
		std::string OrderId; // 订单号
		std::string OrderStatus; // 更新的订单状态
		time_t UpdateTime; // 更新时间
	};

	class Executor // 执行器
	{
	public:
		Executor();

	private:
		Pathfinder::Pathfinder& pathfinder;
		void arbitrageDataHandler(Pathfinder::TransactionPath& path);
	};
}
