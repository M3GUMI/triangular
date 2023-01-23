#pragma once
#include <string>
#include <map>
#include "lib/pathfinder/pathfinder.h"

namespace Arbitrage
{
	class Order // 订单
	{
	public:
		string TaskId;	    // 套利任务Id
		string OrderId;	    // 订单Id
		string OrderStatus; // 订单状态
	private:
	};

	map<string, Order*> orderStore; // 订单存储

	struct SearchOrderResp
	{
		Order *OrderData; // 订单数据
		BaseResp *Base;
	};

	// 三角套利
	class TriangularArbitrage
	{
	public:
		std::string TaskId; // 任务id

		BaseResp *Run(Pathfinder::Pathfinder *pathfinder, Pathfinder::TransactionPath *path);

	private:
		std::string OriginToken; // 原始起点token

		std::string PositionToken; // 当前持仓token，算法起点。todo 当前只计算一种持仓token
		double PositionQuantity;   // 持仓数量

		SearchOrderResp searchOrder(std::string orderId);
	};
}