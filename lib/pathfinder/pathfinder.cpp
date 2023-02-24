#include "utils/utils.h"
#include "conf/conf.h"
#include "pathfinder.h"

using namespace std;
namespace Pathfinder
{
	Pathfinder::Pathfinder(websocketpp::lib::asio::io_service &ioService, HttpWrapper::BinanceApiWrapper &apiWrapper) : ioService(ioService), apiWrapper(apiWrapper)
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
            break;
            if (conf::EnableMock &&
                (item.first != "BTCBUSD" && item.first != "ETHBUSD" && item.first != "ETHBTC" && item.first != "BUSDUSDT" &&
                 item.first != "XRPBTC" && item.first != "FTTBUSD" && item.first != "XRPETH" &&
                 item.first != "LTCBTC" && item.first != "LTCBUSD" && item.first != "LTCETH" &&
                 item.first != "DGBBUSD" && item.first != "DOGEBUSD" && item.first != "XRPBUSD" && item.first != "RUNEBUSD" &&
                 item.first != "MATICBTC" && item.first != "MATICBUSD" && item.first != "MATICETH")) {
                continue;
            }

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

        if (conf::EnableMock) {
            WebsocketWrapper::DepthData val{};
            depthDataHandler(val);
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

    void Pathfinder::HuntingKing() {
        return;
        huntingTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService, websocketpp::lib::asio::milliseconds(200));
        huntingTimer->async_wait(bind(&Pathfinder::HuntingKing, this));

        auto chance = Graph::CalculateArbitrage();
        if (chance.Profit <= 1) {
            return;
        }

        return this->subscriber(chance);
    }

	void Pathfinder::depthDataHandler(WebsocketWrapper::DepthData &data) {
        Graph::AddEdge("BUSD", "BTC", 5, 100, true);
        Graph::AddEdge("BTC", "BUSD", 5.1, 100, false);

        Graph::AddEdge("BTC", "ETH", 10, 50, true);
        Graph::AddEdge("ETH", "BTC", 10.1, 50, false);

        Graph::AddEdge("ETH", "BUSD", 0.025, 10, true);
        Graph::AddEdge("BUSD", "ETH", 0.026, 10, false);

        /*if (!data.Bids.empty()) { // 买单挂出价，我方卖出价
            auto depth = data.Bids[0];
            Graph::AddEdge(data.FromToken, data.ToToken, depth.Price, depth.Quantity, true);
        }
        if (!data.Asks.empty()) { // 卖单挂出价，我方买入价
            auto depth = data.Asks[0];
            Graph::AddEdge(data.ToToken, data.FromToken, depth.Price, 1/depth.Price*depth.Quantitydepth.Quantity, false);
        }*/
    }

	void Pathfinder::SubscribeArbitrage(function<void(ArbitrageChance &path)> handler)
	{
		this->subscriber = handler;
	}

	int Pathfinder::RevisePath(RevisePathReq req, ArbitrageChance &resp)
	{
        auto chance = Graph::FindBestPath(req.Origin, req.End, req.Quantity);
        resp.Profit = chance.Profit;
        resp.Quantity = chance.Quantity;
        resp.Path = chance.Path;
        return 0;
    }
}
