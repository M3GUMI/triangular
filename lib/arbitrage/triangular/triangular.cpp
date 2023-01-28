#include "./defined/defined.h"
#include "triangular.h"
#include "lib/pathfinder/pathfinder.h"
#include "iostream"
#include "string"
#include <chrono>
#include <random>
#include <sstream>
#include "./lib/api/binance.h"
std::stringstream ss;

namespace Arbitrage
{
	BaseResp *TriangularArbitrage::Run(Pathfinder::Pathfinder &pathfinder, Pathfinder::TransactionPath &path)
	{
		Pathfinder::TransactiItemonPath firstPath = path.Path[0];
		ExecuteTrans(pathfinder, firstPath);
		return NewBaseResp("");
	}

	int TriangularArbitrage::ExecuteTrans(Pathfinder::Pathfinder &pathfinder, Pathfinder::TransactiItemonPath &path)
	{
		std::string from = path.FromToken;
		std::string to = path.ToToken;
		double ori = path.FromQuantity;

		Arbitrage::TriangularArbitrage::send(path);
	}

	Arbitrage::Order TriangularArbitrage::TriangularArbitrage::generateOrder(Pathfinder::TransactiItemonPath &path)
	{
		std::string from = path.FromToken;
		std::string to = path.ToToken;
		double fromPrice = path.FromPrice;
		double toPrice = path.ToPrice;
		double fromQuantity = path.FromQuantity;
		double toQuantity = path.ToQuantity;
		std::string OrderId = generateOrderId();

		Arbitrage::Order order = {this->TaskId, OrderId, "CREATED", from, fromPrice, fromQuantity, to, toPrice, toQuantity, "LIMIT", "IOC"};

		if (IsStatic(from) && IsStatic(to))
		{
			order.TimeInForce = "GTC";
		}

		// 记录 order
		Arbitrage::orderStore[OrderId] = &order;

		return order;
	}

	void Arbitrage::TriangularArbitrage::OrderStatusCallback(Arbitrage::OrderResult orderResult)
	{
		std::string from = orderResult.from;
		std::string to = orderResult.to;
		double exec = orderResult.exceQuantity;
		double ori = orderResult.originQuantity;
		std::string status = orderResult.status;

		// IOC 稳定币到x币
		if (IsStatic(from) && not IsStatic(to))
		{
			Static2X(orderResult);
		}

		// IOC x币到x币
		if (not IsStatic(from) && not IsStatic(to))
		{
			X2X(orderResult);
		}

		// IOC x币到稳定币
		else if (not IsStatic(from) && IsStatic(to))
		{
			X2Static(orderResult);
		}

		// GTC 稳定币转稳定币
		else if (IsStatic(from) && IsStatic(to))
		{
			Static2Static(orderResult);
		}
	}

	void Arbitrage::TriangularArbitrage::X2Static(Arbitrage::OrderResult &orderResult)
	{
		std::string OrderId = orderResult.OrderId;
		std::string from = orderResult.from;
		std::string to = orderResult.to;
		double price = orderResult.FromPrice;
		double exce = orderResult.exceQuantity;
		double ori = orderResult.originQuantity;
		std::string status = orderResult.status;
		if (status == "PARTIALLY_FILLED")
		{
			/* 1. 先跑完当前部分的交易 */
			auto it = orderStore.find(OrderId);
			orderStore.erase(it);
			// 调用找路径
			this->pathfinder.RevisePath(to, this->TargetToken, exce*price);

			/* 2. 处理没弄完的不稳定币 递归更新路径并下单 到状态为FILLED才结束 */
			// 继续下单 直到到状态为FILLED才开始下一步
			double remain = ori - exce;
			
			// 调用找路径
			this->pathfinder.RevisePath(from, this->TargetToken, remain);
		}
		else if (status == "FILLED")
		{
			// 删除map中的数据
			auto it = orderStore.find(OrderId);
			orderStore.erase(it);

			// 调用找路径
			this->pathfinder.RevisePath(to, this->TargetToken, exce*price);
		}
	}

	void Arbitrage::TriangularArbitrage::X2X(Arbitrage::OrderResult &orderResult)
	{
		std::string OrderId = orderResult.OrderId;
		std::string from = orderResult.from;
		std::string to = orderResult.to;
		double price = orderResult.FromPrice;
		double exce = orderResult.exceQuantity;
		double ori = orderResult.originQuantity;
		std::string status = orderResult.status;

		if (status == "PARTIALLY_FILLED")
		{
			/* 1. 先跑完当前部分的交易 */
			auto it = orderStore.find(OrderId);
			orderStore.erase(it);
			// 调用找路径
			this->pathfinder.RevisePath(to, this->TargetToken, exce*price);

			/* 2. 处理没弄完的不稳定币 递归更新路径并下单 到状态为FILLED才结束 */
			// 继续下单 直到到状态为FILLED才开始下一步
			double remain = ori - exce;
			
			// 调用找路径
			this->pathfinder.RevisePath(from, this->TargetToken, remain);
		}
		else if (status == "FILLED")
		{
			// 删除map中的数据
			auto it = orderStore.find(OrderId);
			orderStore.erase(it);

			// 调用找路径
			this->pathfinder.RevisePath(to, this->TargetToken, exce*price);
		}
	}

	void Arbitrage::TriangularArbitrage::Static2X(Arbitrage::OrderResult &orderResult)
	{
		std::string OrderId = orderResult.OrderId;
		std::string from = orderResult.from;
		std::string to = orderResult.to;
		double exce = orderResult.exceQuantity;
		double ori = orderResult.originQuantity;
		double price = orderResult.FromPrice;
		std::string status = orderResult.status;
		if (exce > 0)
		{
			// 删除交易map
			auto it = orderStore.find(OrderId);
			orderStore.erase(it);

			// 调用找路径 进行下一步
			this->pathfinder.RevisePath(to, this->TargetToken, exce * price);
		}
		else
		{
			// 把没有换到的消息发给管理器
			cout << "First Step Incomplete" << endl;
		}
	}

	void Arbitrage::TriangularArbitrage::Static2Static(Arbitrage::OrderResult &orderResult)
	{
		std::string OrderId = orderResult.OrderId;
		std::string from = orderResult.from;
		std::string to = orderResult.to;
		double exce = orderResult.exceQuantity;
		double price = orderResult.FromPrice;
		double ori = orderResult.originQuantity;
		std::string status = orderResult.status;

		// 订单没了重发一波
		if (status == "CANCELED" || status == "EXPIRED" || status == "REJECTED")
		{
			// 删除map订单
			auto it = orderStore.find(OrderId);
			orderStore.erase(it);
			Pathfinder::TransactiItemonPath path;
			Arbitrage::TriangularArbitrage::send(path);
		}
		else if (status == "FILLED")
		{
			// 完成三角套利交易，删除订单
			auto it = orderStore.find(OrderId);
			orderStore.erase(it);

			return;
		}
		else if (status == "NEW" || status == "PARTIALLY_FILLED")
		{
			return;
		}
	}

	unsigned long getRand()
	{
		static default_random_engine e(chrono::system_clock::now().time_since_epoch().count() / chrono::system_clock::period::den);
		return e();
	}

	std::string generateOrderId()
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

	binancewebrestapiwrapper::CreateOrderReq Order2OrderReq(Order order){
		binancewebrestapiwrapper::CreateOrderReq createOrderReq = {
			order.OrderId,
			order.FromToken,
			order.FromPrice,
			order.FromQuantity,
			order.ToToken,
			order.ToPrice,
			order.ToQuantity,
			order.OrderType,
			order.TimeInForce
		};
		return createOrderReq;
	}

	void Arbitrage::TriangularArbitrage::send(Pathfinder::TransactiItemonPath p){
		Pathfinder::TransactiItemonPath &path = p;
		Order order = generateOrder(path);
		binancewebrestapiwrapper::CreateOrderReq orderReq = Order2OrderReq(order);
		binance::binance::createOrder(orderReq);
	}

	SearchOrderResp TriangularArbitrage::searchOrder(std::string orderId)
	{
		SearchOrderResp resp;
		resp.Base = NewBaseResp("");

		auto val = orderStore.find(orderId);
		if (val == orderStore.end())
		{
			resp.Base = FailBaseResp("not found order");
			return resp;
		}

		resp.OrderData = val->second;
		if (resp.OrderData == NULL)
		{
			resp.Base = FailBaseResp("order is null");
			return resp;
		}

		return resp;
	}

	bool IsStatic(std::string coinName)
	{
		return Arbitrage::staticCoin.count(coinName);
	}
}