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

	Pathfinder::~Pathfinder()
	{
	}

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
                 item.first != "DGBUSDT" && item.first != "DGBBTC" && item.first != "AMPUSDT" && item.first != "AMPBTC" &&
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
                spdlog::error("func: {}, symbol: {}, err: {}", "scanDepthSocket", symbol, WrapErr((err)));
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

        auto path = Graph::CalculateArbitrage();
        if (path.size() != 3) {
            return;
        }

        double base = 10000; // 验算
        vector<string> info;
        for (const auto& item:path) {
            base = base*item.FromPrice*(1-0.0003);
        }

        if (base <= 10000) {
            spdlog::error("func: HuntingKing, err: path can not get money");
            return;
        }

        ArbitrageChance chance{};
        chance.Profit = base/10000;
        chance.Path = path;
        this->subscriber(chance);
    }

	void Pathfinder::depthDataHandler(WebsocketWrapper::DepthData &data)
	{
        if (data.Asks.size() > 0) {
            Graph::AddEdge(data.FromToken, data.ToToken, data.Asks[0].Price, data.Asks[0].Quantity);
        }
        if (data.Bids.size() > 0) {
            Graph::AddEdge(data.ToToken, data.FromToken, 1/data.Bids[0].Price, data.Bids[0].Quantity);
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
        auto result = Graph::FindBestPath(req.Origin, req.End);
        resp.Profit = result.first;
        resp.Path = result.second;
        return 0;
    }
}
