#include "utils/utils.h"
#include "conf/conf.h"
#include "pathfinder.h"

using namespace std;
namespace Pathfinder
{
	Pathfinder::Pathfinder(websocketpp::lib::asio::io_service &ioService, HttpWrapper::BinanceApiWrapper &apiWrapper) : ioService(ioService), Graph(apiWrapper)
	{
        apiWrapper.SubscribeSymbolReady(bind(&Pathfinder::symbolReadyHandler, this, placeholders::_1));
	}

	Pathfinder::~Pathfinder() = default;

	void Pathfinder::symbolReadyHandler(vector<HttpWrapper::BinanceSymbolData> &data)
    {
        // 每秒检测连接情况 todo 需要增加depth有效性检测，有可能连接存在但数据为空
        scanDepthTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService,
                                                                                websocketpp::lib::asio::milliseconds(
                                                                                        1000));
        scanDepthTimer->async_wait(bind(&Pathfinder::scanDepthSocket, this));

        Graph::Init(data);

        for (const auto& symbolData : data)
        {
            auto symbol = symbolData.Symbol;
//            if (symbol != "XRPBUSD" &&
//                symbol != "XRPBTC" && symbol != "XRPETH" && symbol != "XRPBNB" && symbol != "XRPUSDC" &&
//                symbol != "XRPUSDT" &&
//                symbol != "BTCUSDT" && symbol != "ETHUSDT" && symbol != "BNBUSDT" && symbol != "USDCUSDT" &&
//                symbol != "BUSDUSDT" &&
//
//                symbol != "ARBUSDT" && symbol != "TWTUSDT" &&
//                symbol != "BSWUSDT" && symbol != "FIDAUSDT" && symbol != "TRYUSDT" &&
//                 symbol != "ARBBNB" && symbol != "ARBBUSD" && symbol != "ARBETH"&&symbol != "ARBBTC")
//            {
//                continue;
//            }

            depthSocketMap[symbol] = new WebsocketWrapper::BinanceDepthWrapper(ioService, apiWrapper,
                                                                               "stream.binance.com", "9443");
            if (auto err = (*depthSocketMap[symbol]).Connect(symbol); err > 0)
            {
                spdlog::error("func: {}, symbol: {}, err: {}", "symbolReadyHandler", symbol, WrapErr((err)));
            }
            else
            {
                (*depthSocketMap[symbol]).SubscribeDepth(bind(&Pathfinder::UpdateNode, this, placeholders::_1));
            }
        }
    }

    void Pathfinder::scanDepthSocket() {
        auto initNum = 0, succNum = 0, failNum = 0;
        for (const auto& item:this->depthSocketMap) {
            auto symbol = item.first;
            auto socket = item.second;

            if (socket->Status == define::SocketStatusInit) {
                initNum++;
                continue;
            }

            if (socket->Status == define::SocketStatusConnected) {
                succNum++;
                continue;
            }

            failNum++;
            delete socket;

            depthSocketMap[symbol] = new WebsocketWrapper::BinanceDepthWrapper(ioService, apiWrapper, "stream.binance.com", "9443");
            if (auto err = (*depthSocketMap[symbol]).Connect(symbol); err > 0) {
                spdlog::error("func: scanDepthSocket, symbol: {}, err: {}", symbol, WrapErr((err)));
            } else {
                (*depthSocketMap[symbol]).SubscribeDepth(bind(&Pathfinder::UpdateNode, this, placeholders::_1));
            }
        }

        scanDepthTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService, websocketpp::lib::asio::milliseconds(1000));
        scanDepthTimer->async_wait(bind(&Pathfinder::scanDepthSocket, this));
        if (initNum > 0 || failNum > 0) {
            spdlog::info("func: {}, init: {}, connected: {}, reconnect: {}", "scanDepthSocket", initNum, succNum, failNum);
        }

        if (this->depthSocketMap.size() - succNum <= 10 && this->depthReadySubscriber != nullptr)
        {
            this->depthReadySubscriber();
            this->depthReadySubscriber = nullptr;
        }
    }

    void Pathfinder::SubscribeDepthReady(function<void()> callback)
    {
        this->depthReadySubscriber = callback;
    }
}
