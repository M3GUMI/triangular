#include "iostream"
#include <sstream>
#include "ioc_triangular.h"
#include "utils/utils.h"

namespace Arbitrage
{
	IocTriangularArbitrage::IocTriangularArbitrage(Pathfinder::Pathfinder &pathfinder, CapitalPool::CapitalPool &pool, HttpWrapper::BinanceApiWrapper &apiWrapper) : TriangularArbitrage(pathfinder, pool, apiWrapper)
	{
	}

	IocTriangularArbitrage::~IocTriangularArbitrage()
	{
	}

	int IocTriangularArbitrage::Run(Pathfinder::TransactionPath &path)
	{
		LogInfo("func", "Run", "msg", "IocTriangularArbitrage start");
		Pathfinder::TransactionPathItem firstPath = path.Path[0];
		if (auto err = capitalPool.LockAsset(firstPath.FromToken, firstPath.FromQuantity); err > 0) {
			return err;
		}

		this->OriginQuantity = firstPath.FromQuantity;
		this->OriginToken = firstPath.FromToken;
		this->TargetToken = firstPath.FromToken;
		TriangularArbitrage::ExecuteTrans(firstPath, bind(&IocTriangularArbitrage::ExecuteTransHandler, this, placeholders::_1, placeholders::_2));
		return 0;
	}

	void IocTriangularArbitrage::ExecuteTransHandler(HttpWrapper::OrderData &data, int createErr)
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

	int IocTriangularArbitrage::filledHandler(HttpWrapper::OrderData &data)
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

		auto err = pathfinder.RevisePath(req, resp);
		if (err > 0)
		{
			return err;
		}

		err = TriangularArbitrage::ExecuteTrans(resp.Path.front(), bind(&IocTriangularArbitrage::ExecuteTransHandler, this, placeholders::_1, placeholders::_2));
		if (err > 0)
		{
			return err;
		}

		return 0;
	}

	int IocTriangularArbitrage::partiallyFilledHandler(HttpWrapper::OrderData &data)
	{
		if (define::IsStableCoin(data.FromToken))
		{
			// 稳定币持仓，等待rebalance
			TriangularArbitrage::capitalPool.FreeAsset(data.FromToken, data.ExecuteQuantity);
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

			auto err = TriangularArbitrage::pathfinder.RevisePath(req, resp);
			if (err > 0)
			{
				return err;
			}

			err = TriangularArbitrage::ExecuteTrans(resp.Path.front(), bind(&IocTriangularArbitrage::ExecuteTransHandler, this, placeholders::_1, placeholders::_2));
			if (err > 0)
			{
				return err;
			}

			return 0;
		}
	}
}