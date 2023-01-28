#pragma once
#include <string>
#include <map>
#include "lib/pathfinder/pathfinder.h"
#include <set>

namespace Arbitrage
{
	class Order // 订单
	{
	public:
		std::string TaskId;		// 套利任务Id
		std::string OrderId;		// 订单Id
		std::string OrderStatus; // 订单状态
		std::string FromToken;
		double FromPrice;
		double FromQuantity;
		std::string ToToken;
		double ToPrice;
		double ToQuantity;
		std::string OrderType;
		std::string TimeInForce;
	private:
	};

	class OrderResult
	{
	public:
		std::string OrderId;
		std::string status;
		std::string from;
		std::string to;
		double exceQuantity;
		double originQuantity;
		double FromPrice;
	};

	map<std::string, Order*> orderStore; // 订单存储
	set<std::string> staticCoin;

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
		Pathfinder::Pathfinder pathfinder;
		std::string OriginToken; // 原始起点token
		std::string TargetToken;
		BaseResp *Run(Pathfinder::Pathfinder &pathfinder, Pathfinder::TransactionPath &path);
		Arbitrage::Order TriangularArbitrage::TriangularArbitrage::generateOrder(Pathfinder::TransactiItemonPath &path);
		int TriangularArbitrage::ExecuteTrans(Pathfinder::Pathfinder& pathfinder, Pathfinder::TransactiItemonPath& path);
		Arbitrage::Order TriangularArbitrage::TriangularArbitrage::generateOrder(Pathfinder::TransactiItemonPath& path);
		void Arbitrage::TriangularArbitrage::OrderStatusCallback(Arbitrage::OrderResult orderResult);

	private:

		SearchOrderResp searchOrder(std::string orderId);
		void X2Static(Arbitrage::OrderResult &orderResult);
		void X2X(Arbitrage::OrderResult &orderResult);
		void Static2X(Arbitrage::OrderResult &orderResult);
		void Static2Static(Arbitrage::OrderResult &orderResult);
		void Arbitrage::TriangularArbitrage::send(Pathfinder::TransactiItemonPath p);
	};
}