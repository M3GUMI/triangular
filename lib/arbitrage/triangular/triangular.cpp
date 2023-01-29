#include "iostream"
#include <chrono>
#include <random>
#include <sstream>
#include "triangular.h"
#include "define/define.h"
#include "lib/pathfinder/pathfinder.h"
#include "lib/api/api.h"

std::stringstream ss;
namespace Arbitrage
{
	TriangularArbitrage::TriangularArbitrage(Pathfinder::Pathfinder &pathfinder): pathfinder(pathfinder)
	{
	}

	TriangularArbitrage::~TriangularArbitrage()
	{
	}

	int TriangularArbitrage::Run(Pathfinder::TransactionPath &path)
	{
		Pathfinder::TransactionPathItem firstPath = path.Path[0];
		this->OriginToken = firstPath.FromToken;

		ExecuteTrans(firstPath);
		return 0;
	}

	int TriangularArbitrage::ExecuteTrans(Pathfinder::TransactionPathItem &path)
	{
		HttpWrapper::CreateOrderReq req;
		req.FromToken = path.FromToken;
		req.FromPrice = path.FromPrice;
		req.FromQuantity = path.FromQuantity;
		req.ToToken = path.ToToken;
		req.ToPrice = path.ToPrice;
		req.ToQuantity = path.ToQuantity;
		req.OrderType = define::LIMIT;
		req.TimeInForce = define::IOC;
		auto callback = bind(&TriangularArbitrage::orderDataHandler, this, placeholders::_1);

		API::GetBinanceApiWrapper().CreateOrder(req, callback);
		// Arbitrage::orderStore[OrderId] = &order;
	}

	void TriangularArbitrage::orderDataHandler(HttpWrapper::OrderData& data)
	{
		int ec;
		if (data.OrderStatus == define::FILLED)
		{
			ec = filledHandler(data);
		}
		else if (data.OrderStatus == define::PARTIALLY_FILLED)
		{
			ec = partiallyFilledHandler(data);
		}
		else
		{
			return;
		}

		if (ec > 0)
		{
			// todo error处理
			return;
		}
	}

	int TriangularArbitrage::filledHandler(HttpWrapper::OrderData &data)
	{
		if (data.ToToken == this->TargetToken) {
			// 套利完成
			return 0;
		}

		// 执行下一步路径
		Pathfinder::RevisePathReq req;
		Pathfinder::TransactionPath resp;

		req.Origin = data.ToToken;
		req.End = this->TargetToken;
		req.PositionQuantity = data.ExecuteQuantity * data.ExecutePrice;

		auto ec = this->pathfinder.RevisePath(req, resp);
		if (ec > 0)
		{
			// todo error日志处理
			return ec;
		}

		ec = ExecuteTrans(resp.Path.front());
		if (ec > 0)
		{
			// todo error日志处理
			return ec;
		}

		return 0;
	}

	int TriangularArbitrage::partiallyFilledHandler(HttpWrapper::OrderData &data)
	{
		if (define::IsStableCoin(data.FromToken))
		{
			// 稳定币持仓，等待rebalance
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

			auto ec = this->pathfinder.RevisePath(req, resp);
			if (ec > 0)
			{
				// todo error日志处理
				return ec;
			}

			ec = ExecuteTrans(resp.Path.front());
			if (ec > 0)
			{
				// todo error日志处理
				return ec;
			}

			return 0;
		}
	}

	int TriangularArbitrage::SearchOrder(string orderId, SearchOrderResp &resp)
	{
		auto val = orderStore.find(orderId);
		if (val == orderStore.end())
		{
			return 1;
		}

		resp.OrderData = val->second;
		if (resp.OrderData == NULL)
		{
			return 1;
		}

		return 0;
	}

	unsigned long getRand()
	{
		static default_random_engine e(chrono::system_clock::now().time_since_epoch().count() / chrono::system_clock::period::den);
		return e();
	}

	string TriangularArbitrage::generateOrderId()
	{
		std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch());

		// 获取 long long的OrderId
		unsigned long long timestamp = ms.count();
		unsigned long long longOrderId = (timestamp << 32 | getRand());

		// 转long long 转 string
		stringstream stream;
		string result;

		stream << longOrderId;
		stream >> result;
		return result;
	}
}