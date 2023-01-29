#pragma once
#include <string>
#include <map>
#include <set>
#include "lib/pathfinder/pathfinder.h"
#include "define/define.h"

using namespace std;
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

	map<std::string, Order*> orderStore; // 订单存储

	struct SearchOrderResp
	{
		Order *OrderData; // 订单数据
	};

	// 三角套利
	class TriangularArbitrage
	{
	public:
		TriangularArbitrage(Pathfinder::Pathfinder &pathfinder);
		~TriangularArbitrage();

		int Run(Pathfinder::TransactionPath &path);
		int ExecuteTrans(Pathfinder::TransactionPathItem &path);
		int SearchOrder(string orderId, SearchOrderResp& resp);
	private:
		Pathfinder::Pathfinder &pathfinder;
		string TaskId;		// 任务id
		string OriginToken; // 原始起点token
		string TargetToken; // 目标token
		void orderDataHandler(HttpWrapper::OrderData &orderData);
		int filledHandler(HttpWrapper::OrderData &data);
		int partiallyFilledHandler(HttpWrapper::OrderData &data);

		string generateOrderId();
	};
}