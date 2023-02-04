#pragma once
#include <string>
#include <time.h>
#include <functional>
#include "lib/api/api.h"

using namespace std;
namespace Pathfinder
{
	// 套利路径项
	struct TransactionPathItem
	{
		std::string FromToken; // 卖出的token
		double FromPrice;	   // 卖出价格
		double FromQuantity;   // 卖出数量
		std::string ToToken;   // 买入的token
		double ToPrice;		   // 买入价格
		double ToQuantity;	   // 买入数量
	};

	struct TransactionPath
	{
		vector<TransactionPathItem> Path;
	};

	struct RevisePathReq
	{
		string Origin;
		string End;
		double PositionQuantity;
	};

	struct GetExchangePriceReq
	{
		string FromToken;
		string ToToken;
	};

	struct GetExchangePriceResp
	{
		double FromPrice;
		double ToPrice;
	};

	class Pathfinder
	{
	private:
		function<void(TransactionPath &path)> subscriber = NULL;

		void depthDataHandler(WebsocketWrapper::DepthData& data); // 接收depth数据处理

	public:
		Pathfinder(); 
		~Pathfinder(); 

		void SubscribeArbitrage(function<void(TransactionPath &path)> handler); // 订阅套利机会推送
		int RevisePath(RevisePathReq req, TransactionPath& resp);				// 路径修正
		int GetExchangePrice(GetExchangePriceReq& req, GetExchangePriceResp& resp);				// 路径修正
	};

    extern Pathfinder* pathfinder;
    void Init();
    Pathfinder& GetPathfinder();
}