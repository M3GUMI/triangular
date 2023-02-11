#pragma once
#include <string>
#include <map>
#include <time.h>
#include <functional>
#include "lib/api/ws/binance_depth_wrapper.h"

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
        map<string, WebsocketWrapper::BinanceDepthWrapper*> depthSocketMap; // depth连接管理
        std::shared_ptr<websocketpp::lib::asio::steady_timer> scanDepthTimer; // depth检查计时器

		function<void(TransactionPath &path)> subscriber = nullptr;
		HttpWrapper::BinanceApiWrapper &apiWrapper;
		websocketpp::lib::asio::io_service &ioService;

		void symbolReadyHandler(map<string, HttpWrapper::BinanceSymbolData> &data); // symbol数据就绪
		void depthDataHandler(WebsocketWrapper::DepthData &data);					// 接收depth数据处理
        void scanDepthSocket();                                                     // 检查socket连接有效性
	public:
		Pathfinder(websocketpp::lib::asio::io_service &ioService, HttpWrapper::BinanceApiWrapper &apiWrapper);
		~Pathfinder();

		void SubscribeArbitrage(function<void(TransactionPath &path)> handler);		// 订阅套利机会推送
		int RevisePath(RevisePathReq req, TransactionPath &resp);					// 路径修正
		int GetExchangePrice(GetExchangePriceReq &req, GetExchangePriceResp &resp); // 路径修正
	};
}