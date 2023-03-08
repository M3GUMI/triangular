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
    class Pathfinder : public Graph
    {
    private:
        websocketpp::lib::asio::io_service& ioService;

        map<string, WebsocketWrapper::BinanceDepthWrapper*> depthSocketMap; // depth连接管理
        std::shared_ptr<websocketpp::lib::asio::steady_timer> scanDepthTimer; // depth检查计时器
        std::shared_ptr<websocketpp::lib::asio::steady_timer> huntingTimer; // 寻找套利计时器

        void symbolReadyHandler(vector<HttpWrapper::BinanceSymbolData>& data); // symbol数据就绪
        void scanDepthSocket(); // 检查socket连接有效性
    public:
        Pathfinder(websocketpp::lib::asio::io_service& ioService, HttpWrapper::BinanceApiWrapper& apiWrapper);

        ~Pathfinder();
    };
}