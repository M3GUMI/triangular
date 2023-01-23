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
		string TaskId;		// 套利任务Id
		string OrderId;		// 订单Id
		string OrderStatus; // 订单状态
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

	map<string, Order*> orderStore; // 订单存储
	set<string> staticCoin;

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

		TriangularArbitrage();
		TriangularArbitrage(std::string TaskId, std::string OriginToken, Pathfinder::Pathfinder pathfinder){
			this->TaskId = TaskId;
			this->OriginToken = OriginToken;
			this->pathfinder = pathfinder;
		};
		BaseResp *Run(Pathfinder::Pathfinder &pathfinder, Pathfinder::TransactionPath &path);
		std::string generateOrderId();
		int TriangularArbitrage::ExecuteTrans(Pathfinder::Pathfinder& pathfinder, Pathfinder::TransactiItemonPath& path);
		void TriangularArbitrage::setPositionToken(std::string PositionToken);
		void TriangularArbitrage::setPositionQuantity(double PositionQuantity);
		Arbitrage::Order TriangularArbitrage::TriangularArbitrage::generateOrder(Pathfinder::TransactiItemonPath& path);
		void Arbitrage::TriangularArbitrage::IOCOrderStatusHandler(Arbitrage::OrderResult orderResult);
		void Arbitrage::TriangularArbitrage::GTCOrderStatusHandler(Arbitrage::OrderResult orderResult);

	private:
		std::string OriginToken; // 原始起点token

		std::string PositionToken; // 当前持仓token，算法起点。todo 当前只计算一种持仓token
		double PositionQuantity;   // 持仓数量

		SearchOrderResp searchOrder(std::string orderId);
		void X2Static(Arbitrage::OrderResult &orderResult);
		void X2X(Arbitrage::OrderResult &orderResult);
		void Static2X(Arbitrage::OrderResult &orderResult);
		void Static2Static(Arbitrage::OrderResult &orderResult);
	};
}