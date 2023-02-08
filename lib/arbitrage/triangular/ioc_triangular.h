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
	// 两次ioc三角套利
	class IocTriangularArbitrage : public TriangularArbitrage
	{
	public:
		IocTriangularArbitrage(Pathfinder::Pathfinder &pathfinder, CapitalPool::CapitalPool &pool, HttpWrapper::BinanceApiWrapper &apiWrapper);
		~IocTriangularArbitrage();

		int Run(Pathfinder::TransactionPath &path);
	private:
		string OriginToken;	   // 原始起点token
		double OriginQuantity; // 原始起点token数量
		string TargetToken;	   // 目标token

		void ExecuteTransHandler(HttpWrapper::OrderData &orderData, int err);
		int filledHandler(HttpWrapper::OrderData &data);
		int partiallyFilledHandler(HttpWrapper::OrderData &data);
	};
}