#include "triangular.h"
#include "lib/pathfinder/pathfinder.h"

namespace Arbitrage
{
	int TriangularArbitrage::Run(Pathfinder::Pathfinder *pathfinder, Pathfinder::TransactionPath *path)
	{
		if (pathfinder == NULL)
		{
			return 1; //error
		}

                // 1. 执行第一步交易
		//   a. ioc 下单。订阅order status
		// 2. orderStatus更新成功，触发第二步。查询第二步交易路径
		pathfinder->RevisePath("ETH", "USDT");
		// 3. 执行第二步交易
		// 4. maker卖出稳定币
		// 5. 回归USDT
		return 0;
	}

	int TriangularArbitrage::searchOrder(std::string orderId, SearchOrderResp& resp)
	{
		auto val = orderStore.find(orderId);
		if (val == orderStore.end())
		{
			return 1;
		}

		resp.OrderData = val->second;
		if (resp.OrderData == NULL)
		{
			return 1;
		}

		return 0;
	}
}