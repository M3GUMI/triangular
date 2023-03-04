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

	void Pathfinder::symbolReadyHandler(map<string, HttpWrapper::BinanceSymbolData> &data) {
        // 每秒检测连接情况 todo 需要增加depth有效性检测，有可能连接存在但数据为空
        scanDepthTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService, websocketpp::lib::asio::milliseconds(1000));
        scanDepthTimer->async_wait(bind(&Pathfinder::scanDepthSocket, this));

        // 5秒后开始每100ms跑算法
        huntingTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService, websocketpp::lib::asio::milliseconds(5000));
        huntingTimer->async_wait(bind(&Pathfinder::HuntingKing, this));

        for (const auto &item: data) {
            auto symbol = item.first;
            auto symbolData = item.second;

            if (symbol != symbolData.BaseToken + symbolData.QuoteToken) {
                continue;
            }

            depthSocketMap[symbol] = new WebsocketWrapper::BinanceDepthWrapper(ioService, apiWrapper, "stream.binance.com", "9443");
            if (auto err = (*depthSocketMap[symbol]).Connect(symbol); err > 0) {
                spdlog::error("func: {}, symbol: {}, err: {}", "symbolReadyHandler", symbol, WrapErr((err)));
            } else {
                (*depthSocketMap[symbol]).SubscribeDepth(bind(&Pathfinder::depthDataHandler, this, placeholders::_1));
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
                (*depthSocketMap[symbol]).SubscribeDepth(bind(&Pathfinder::depthDataHandler, this, placeholders::_1));
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
                                                                              websocketpp::lib::asio::milliseconds(100));
        huntingTimer->async_wait(bind(&Pathfinder::HuntingKing, this));

        auto time = GetNowTime();
        auto chance = Graph::CalculateArbitrage("IocTriangular");
        if (chance.Profit <= 1)
        {
            return;
        }
        spdlog::info("CalculateArbitrage time: {}ms", GetNowTime() - time);

        return this->subscriber(chance);
    }

	void Pathfinder::depthDataHandler(WebsocketWrapper::DepthData &data) {
        // Graph::AddEdge("BUSD", "BTC", 5, 100, true);
        // Graph::AddEdge("BTC", "BUSD", 5.1, 100, false);

        // Graph::AddEdge("BTC", "ETH", 10, 50, true);
        // Graph::AddEdge("ETH", "BTC", 10.1, 50, false);

        // Graph::AddEdge("ETH", "BUSD", FormatDoubleV2(0.025), 10, true);
        // Graph::AddEdge("BUSD", "ETH", FormatDoubleV2(0.026), 10, false;

        auto symbolData = apiWrapper.GetSymbolData(data.FromToken, data.ToToken);
        if (!data.Bids.empty()) { // 买单挂出价，我方卖出价
            auto depth = data.Bids[0];
            Graph::AddEdge(data.FromToken, data.ToToken, depth.Price, depth.Quantity, symbolData.MinNotional, true);
        }
        if (!data.Asks.empty()) { // 卖单挂出价，我方买入价
            auto depth = data.Asks[0];
            Graph::AddEdge(data.ToToken, data.FromToken, depth.Price, depth.Quantity, symbolData.MinNotional, false);
        }
    }

	void Pathfinder::SubscribeArbitrage(function<void(ArbitrageChance &path)> handler)
	{
		this->subscriber = handler;
	}
}
