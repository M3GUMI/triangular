#include "iostream"
#include <sstream>
#include "triangular.h"
#include "define/define.h"
#include "utils/utils.h"
#include "lib/pathfinder/pathfinder.h"
#include "lib/capital_pool/capital_pool.h"
#include "lib/api/api.h"

std::stringstream ss;
namespace Arbitrage
{
	TriangularArbitrage::TriangularArbitrage()
	{
	}

	TriangularArbitrage::~TriangularArbitrage()
	{
	}

	int TriangularArbitrage::Run(Pathfinder::TransactionPath &path)
	{
		LogInfo("func", "Run", "msg", "TriangularArbitrage start");
		Pathfinder::TransactionPathItem firstPath = path.Path[0];
		CapitalPool::GetCapitalPool().LockAsset(firstPath.FromToken, firstPath.FromQuantity);

		this->OriginQuantity = firstPath.FromQuantity;
		this->TargetToken = firstPath.FromToken;
		ExecuteTrans(firstPath);
		return 0;
	}

	int TriangularArbitrage::Finish(int finalQuantiy)
	{
		LogInfo("func", "Finish", "finalQuantity", to_string(finalQuantiy), "originQuantity", to_string(this->OriginQuantity));
		CapitalPool::GetCapitalPool().Refresh();
		this->subscriber();
		return 0;
	}

	void TriangularArbitrage::SubscribeFinish(function<void()> callback)
	{
		this->subscriber = callback;
	}

	int TriangularArbitrage::ExecuteTrans(Pathfinder::TransactionPathItem &path)
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
		auto callback = bind(&TriangularArbitrage::orderDataHandler, this, placeholders::_1, placeholders::_2);

		API::GetBinanceApiWrapper().CreateOrder(req, callback);
	}

	void TriangularArbitrage::orderDataHandler(HttpWrapper::OrderData &data, int createErr)
	{
		if (createErr > 0)
		{
			return;
		}

		int err;
		if (data.OrderStatus == define::FILLED)
		{
			err = filledHandler(data);
		}
		else if (data.OrderStatus == define::PARTIALLY_FILLED)
		{
			err = partiallyFilledHandler(data);
		}
		else
		{
			return;
		}

		if (err > 0)
		{
			LogError("func", "orderDataHandler", "err", WrapErr(err));
		}
	}

	int TriangularArbitrage::filledHandler(HttpWrapper::OrderData &data)
	{
		if (data.ToToken == this->TargetToken)
		{
			// 套利完成
			return Finish(data.ExecuteQuantity * data.ExecutePrice);
		}

		// 执行下一步路径
		Pathfinder::RevisePathReq req;
		Pathfinder::TransactionPath resp;

		req.Origin = data.ToToken;
		req.End = this->TargetToken;
		req.PositionQuantity = data.ExecuteQuantity * data.ExecutePrice;

		auto err = Pathfinder::GetPathfinder().RevisePath(req, resp);
		if (err > 0)
		{
			return err;
		}

		err = ExecuteTrans(resp.Path.front());
		if (err > 0)
		{
			return err;
		}

		return 0;
	}

	int TriangularArbitrage::partiallyFilledHandler(HttpWrapper::OrderData &data)
	{
		if (define::IsStableCoin(data.FromToken))
		{
			// 稳定币持仓，等待rebalance
			CapitalPool::GetCapitalPool().FreeAsset(data.FromToken, data.ExecuteQuantity);
			return 0;
		}
		else if (define::NotStableCoin(data.FromToken))
		{
			// 寻找新路径重试
			Pathfinder::RevisePathReq req;
			Pathfinder::TransactionPath resp;

			req.Origin = data.FromToken;
			req.End = this->TargetToken;
			req.PositionQuantity = data.OriginQuantity - data.ExecuteQuantity;

			auto err = Pathfinder::GetPathfinder().RevisePath(req, resp);
			if (err > 0)
			{
				return err;
			}

			err = ExecuteTrans(resp.Path.front());
			if (err > 0)
			{
				return err;
			}

			return 0;
		}
	}
}