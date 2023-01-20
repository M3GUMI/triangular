#include "defined/defined.h"
#include "triangular.h"
#include "lib/pathfinder/pathfinder.h"

namespace Arbitrage
{
	BaseResp *TriangularArbitrage::Run(Pathfinder::Pathfinder *pathfinder, Pathfinder::TransactionPath *path)
	{
		if (pathfinder == NULL)
		{
			return FailBaseResp("pathfinder is null");
		}

                // 1. 执行第一步交易
		//   a. ioc 下单。订阅order status
		//   b. 实时价格偏差过大则回滚
		// 2. 查询第二步交易路径
		pathfinder->RevisePath("ETH", "USDT");
		// 3. 执行第二步交易
		// 4. maker卖出稳定币
		// 5. 回归USDT
		return NewBaseResp("");
	}

	SearchOrderResp TriangularArbitrage::searchOrder(std::string orderId)
	{
		SearchOrderResp resp;
		resp.Base = NewBaseResp("");

		auto val = orderStore.find(orderId);
		if (val == orderStore.end())
		{
			resp.Base = FailBaseResp("not found order");
			return resp;
		}

		resp.OrderData = val->second;
		if (resp.OrderData == NULL)
		{
			resp.Base = FailBaseResp("order is null");
			return resp;
		}

		return resp;
	}
}