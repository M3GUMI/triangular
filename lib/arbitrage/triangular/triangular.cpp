#include "triangular.h"
#include "utils/utils.h"

namespace Arbitrage
{
	TriangularArbitrage::TriangularArbitrage(Pathfinder::Pathfinder &pathfinder, CapitalPool::CapitalPool &pool, HttpWrapper::BinanceApiWrapper &apiWrapper) : pathfinder(pathfinder), capitalPool(pool), apiWrapper(apiWrapper)
	{
	}

	TriangularArbitrage::~TriangularArbitrage()
	{
	}

	int TriangularArbitrage::Finish(int finalQuantity)
	{
		LogInfo("func", "Finish", "finalQuantity", to_string(finalQuantity), "originQuantity", to_string(this->OriginQuantity));
		capitalPool.Refresh();
		this->subscriber();
		return 0;
	}

	void TriangularArbitrage::SubscribeFinish(function<void()> callback)
	{
		this->subscriber = callback;
	}

	int TriangularArbitrage::ExecuteTrans(Pathfinder::TransactionPathItem &path, function<void(HttpWrapper::OrderData &data, int createErr)> callback)
	{
		HttpWrapper::CreateOrderReq req;
		req.OrderId = GenerateId();
		req.FromToken = path.FromToken;
		req.FromPrice = path.FromPrice;
		req.FromQuantity = path.FromQuantity;
		req.ToToken = path.ToToken;
		req.ToPrice = path.ToPrice;
		req.ToQuantity = path.ToQuantity;
		req.OrderType = define::LIMIT;
		req.TimeInForce = define::IOC;

		LogInfo("func", "ExecuteTrans", "from", path.FromToken, "to", path.ToToken);
		LogInfo("price", to_string(path.FromPrice), "quantity", to_string(path.FromQuantity));
		apiWrapper.CreateOrder(req, callback);
	}
}