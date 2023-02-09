#pragma once
#include <string>
#include <map>
#include <set>
#include "lib/pathfinder/pathfinder.h"
#include "lib/capital_pool/capital_pool.h"
#include "lib/api/http/binance_api_wrapper.h"
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
		void SubscribeFinish(function<void()> callback);

	protected:
		TriangularArbitrage(Pathfinder::Pathfinder &pathfinder, CapitalPool::CapitalPool &pool, HttpWrapper::BinanceApiWrapper &apiWrapper);
		~TriangularArbitrage();

		string OriginToken;	   // 原始起点token
		double OriginQuantity; // 原始起点token数量
		string TargetToken;	   // 目标token

		Pathfinder::Pathfinder &pathfinder;
		CapitalPool::CapitalPool &capitalPool;
		HttpWrapper::BinanceApiWrapper &apiWrapper;

		int ExecuteTrans(Pathfinder::TransactionPathItem &path, function<void(HttpWrapper::OrderData &data, int createErr)> callback);
		int Finish(int finalQuantiy);

	private:
		function<void()> subscriber = NULL;
	};
}