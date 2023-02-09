#pragma once
#include "triangular.h"

using namespace std;
namespace Arbitrage
{
	// 两次maker挂单三角套利
	class MakerTriangularArbitrage : public TriangularArbitrage
	{
	public:
		MakerTriangularArbitrage(Pathfinder::Pathfinder &pathfinder, CapitalPool::CapitalPool &pool, HttpWrapper::BinanceApiWrapper &apiWrapper);
		~MakerTriangularArbitrage();

		int Run(Pathfinder::TransactionPath &path);

	private:
		void ExecuteTransHandler(HttpWrapper::OrderData &orderData, int err);
	};
}