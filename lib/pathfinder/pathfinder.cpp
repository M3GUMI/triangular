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
            if (conf::EnableMock &&
                (item.first != "BTCUSDT" && item.first != "ETHUSDT" && item.first != "ETHBTC" &&
                 item.first != "XRPBTC" && item.first != "XRPUSDT" && item.first != "XRPETH" &&
                 item.first != "LTCBTC" && item.first != "LTCUSDT" && item.first != "LTCETH" &&
                 item.first != "DGBUSDT" && item.first != "DGBBTC" && item.first != "RUNEGBP" && item.first != "RUNEUSDT" &&
                 item.first != "MATICBTC" && item.first != "MATICUSDT" && item.first != "MATICETH")) {
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
        huntingTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService, websocketpp::lib::asio::milliseconds(200));
        huntingTimer->async_wait(bind(&Pathfinder::HuntingKing, this));

        auto chance = Graph::CalculateArbitrage();
        if (chance.Profit <= 1) {
            return;
        }

        return this->subscriber(chance);
    }

	void Pathfinder::depthDataHandler(WebsocketWrapper::DepthData &data)
	{
        /*Graph::AddEdge("USDT", "BTC", 4.928536, 100);
        Graph::AddEdge("BTC", "USDT", 1/4.928536, 100);

        Graph::AddEdge("BTC", "ETH", 3.92, 50);
        Graph::AddEdge("ETH", "BTC", 1/3.92, 50);

        Graph::AddEdge("ETH", "USDT", 0.051813, 10);
        Graph::AddEdge("USDT", "ETH", 1/0.051813, 10);*/

        if (!data.Bids.empty()) { // 买单挂出价，我方卖出价
            auto depth = data.Bids[0];
            Graph::AddEdge(data.FromToken, data.ToToken, depth.Price, depth.Quantity);
        }
        if (!data.Asks.empty()) { // 卖单挂出价，我方买入价
            auto depth = data.Asks[0];
            Graph::AddEdge(data.ToToken, data.FromToken, 1/depth.Price, depth.Price*depth.Quantity);
        }
	}

	void Pathfinder::SubscribeArbitrage(function<void(ArbitrageChance &path)> handler)
	{
		this->subscriber = handler;
	}

	int Pathfinder::RevisePath(RevisePathReq req, ArbitrageChance &resp)
	{
		// 1. 在负权图中计算路径
		// 2. 返回下一步交易路径
        auto chance = Graph::FindBestPath(req.Origin, req.End);
        resp.Profit = chance.Profit;
        resp.Quantity = chance.Quantity;
        resp.Path = chance.Path;
        return 0;
    }
}
