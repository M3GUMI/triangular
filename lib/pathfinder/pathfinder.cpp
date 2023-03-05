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

	void Pathfinder::symbolReadyHandler(vector<HttpWrapper::BinanceSymbolData> &data) {
        // 每秒检测连接情况 todo 需要增加depth有效性检测，有可能连接存在但数据为空
        scanDepthTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService, websocketpp::lib::asio::milliseconds(1000));
        scanDepthTimer->async_wait(bind(&Pathfinder::scanDepthSocket, this));

        // 5秒后开始每100ms跑算法
        huntingTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService, websocketpp::lib::asio::milliseconds(5000));
        huntingTimer->async_wait(bind(&Pathfinder::HuntingKing, this));

        Graph::Init(data);

        for (const auto &symbolData: data) {
            auto symbol = symbolData.Symbol;

            depthSocketMap[symbol] = new WebsocketWrapper::BinanceDepthWrapper(ioService, apiWrapper, "stream.binance.com", "9443");
            if (auto err = (*depthSocketMap[symbol]).Connect(symbol); err > 0) {
                spdlog::error("func: {}, symbol: {}, err: {}", "symbolReadyHandler", symbol, WrapErr((err)));
            } else {
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
    }

    void Pathfinder::HuntingKing()
    {
        huntingTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService,
                                                                              websocketpp::lib::asio::milliseconds(20));
        huntingTimer->async_wait(bind(&Pathfinder::HuntingKing, this));

        auto time = GetMicroTime();
        auto chance = Graph::CalculateArbitrage("IocTriangular");
        if (chance.Profit <= 1)
        {
            return;
        }
        spdlog::info("CalculateArbitrage time: {}ms", GetNowTime() - time);

        return this->subscriber(chance);
    }

	void Pathfinder::SubscribeArbitrage(function<void(ArbitrageChance &path)> handler)
	{
		this->subscriber = handler;
	}
}
