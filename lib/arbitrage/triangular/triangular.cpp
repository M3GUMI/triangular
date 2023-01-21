#include "./defined/defined.h"
#include "triangular.h"
#include "lib/pathfinder/pathfinder.h"
#include "iostream"
#include "string"
#include <sstream>std::stringstream ss

namespace Arbitrage
{
	struct CreateOrder{
		CreateOrder(
			Pathfinder::TransactiItemonPath path,
			std::string OrderType,
			std::string TimeInForce){
			this->FromToken = path.FromToken;
			this->FromPrice = path.FromPrice;
			this->FromQuantity = path.FromQuantity;
			this->ToToken = path.ToToken;
			this->ToPrice = path.ToPrice;
			this->ToQuantity = path.ToQuantity;
			this->OrderType = OrderType;
			this->TimeInForce = TimeInForce;
		}

		std::string FromToken;
		double FromPrice;
		double FromQuantity;
		std::string ToToken;
		double ToPrice;
		double ToQuantity;
		std::string OrderType;
		std::string TimeInForce;
	};

	BaseResp *TriangularArbitrage::Run(Pathfinder::Pathfinder& pathfinder, Pathfinder::TransactionPath& path)
	{

        // 1. 执行第一步交易
		Pathfinder::TransactiItemonPath firstPath = path.Path[0];
		TakerHandler(firstPath);
			//CreateOrder
		//   a. ioc 下单。订阅order status
		// 2. orderStatus更新成功，触发第二步。查询第二步交易路径
		pathfinder.RevisePath("ETH", "USDT");
		// 3. 执行第二步交易
		//TakeHandler()
		// 4. maker卖出稳定币
		// 5. 回归USDT
		return NewBaseResp("");
	}

	std::string generateOrderId(){
		long longOrderId = time(NULL) << 32 | rand();
		stringstream stream;
		string result;

		stream<<longOrderId;
		stream>>result;
		return result;
	}

	int TriangularArbitrage::TakerHandler(Pathfinder::TransactiItemonPath path){
		CreateOrder createorder1(path, "LIMIT", "IOC");
		Arbitrage::Order order = {"Hand",generateOrderId(),"Status"};

		OrderInformation information = {"NEW", 0, 0};
		//创建线程调用请求，发送回调函数
		//join
		
		string status = information.status;
		if (status == "NEW"){

		} if (status == "PARTIALLY_FILLED"){
			this->PositionToken = path.ToToken;
			this->PositionQuantity = path.ToQuantity;
		} if (status == "FILLED") {
			this->PositionToken = path.ToToken;
			this->PositionQuantity = path.ToQuantity;
		} if (status == "CANCELED") {

		} if (status == "REJECTED") {

		} if (status == "EXPIRED") {

		}

		return 0;
	}

	int TriangularArbitrage::MakerHandler(Pathfinder::TransactiItemonPath path){

	}

	int TriangularArbitrage::SubscribeOrder(Arbitrage::OrderInformation& information){
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

	void TriangularArbitrage::setPositionToken(std::string PositionToken){
		this->PositionToken = PositionToken;
	}

	void TriangularArbitrage::setPositionQuantity(double PositionQuantity){
		this->PositionQuantity = PositionQuantity;
	}
}