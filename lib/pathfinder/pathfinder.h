#pragma once
#include <string>
#include <map>
#include <time.h>
#include <functional>
#include "lib/api/ws/binance_depth_wrapper.h"
#include "graph.h"

using namespace std;
namespace Pathfinder
{
	struct RevisePathReq
	{
		string Origin;
		string End;
		double PositionQuantity;
	};

    class Pathfinder: public Graph {
    private:
        function<void(TransactionPath &path)> subscriber = nullptr;
        HttpWrapper::BinanceApiWrapper &apiWrapper;
        websocketpp::lib::asio::io_service &ioService;

        map<string, WebsocketWrapper::BinanceDepthWrapper *> depthSocketMap; // depth连接管理
        std::shared_ptr<websocketpp::lib::asio::steady_timer> scanDepthTimer; // depth检查计时器
        std::shared_ptr<websocketpp::lib::asio::steady_timer> huntingTimer; // 寻找套利计时器

        void symbolReadyHandler(map<string, HttpWrapper::BinanceSymbolData> &data); // symbol数据就绪
        void depthDataHandler(WebsocketWrapper::DepthData &data);                    // 接收depth数据处理
        void scanDepthSocket();                                                     // 检查socket连接有效性
    public:
        Pathfinder(websocketpp::lib::asio::io_service &ioService, HttpWrapper::BinanceApiWrapper &apiWrapper);
        ~Pathfinder();

        void HuntingKing();
        void SubscribeArbitrage(function<void(TransactionPath &path)> handler);        // 订阅套利机会推送
        int RevisePath(RevisePathReq req, TransactionPath &resp);                    // 路径修正
    };
}