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
		std::string TaskId;		 // 套利任务Id
		std::string OrderId;	 // 订单Id
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

	// 三角套利
	class TriangularArbitrage
	{
	public:
		TriangularArbitrage();
		~TriangularArbitrage();

		int Run(Pathfinder::TransactionPath &path);
		int ExecuteTrans(Pathfinder::TransactionPathItem &path);
		void SubscribeFinish(function<void()> callback);
	private:
		function<void()> subscriber = NULL;

		string OriginToken;	   // 原始起点token
		double OriginQuantity; // 原始起点token数量
		string TargetToken;	   // 目标token

		int Finish(int finalQuantiy);
		void orderDataHandler(HttpWrapper::OrderData &orderData, int err);
		int filledHandler(HttpWrapper::OrderData &data);
		int partiallyFilledHandler(HttpWrapper::OrderData &data);
	};
}