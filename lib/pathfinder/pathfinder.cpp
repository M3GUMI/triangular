#include "utils/utils.h"
#include "conf/conf.h"
#include "pathfinder.h"

using namespace std;
namespace Pathfinder
{
	Pathfinder::Pathfinder(websocketpp::lib::asio::io_service &ioService, HttpWrapper::BinanceApiWrapper &apiWrapper) : ioService(ioService), apiWrapper(apiWrapper)
	{
        huntingTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService, websocketpp::lib::asio::milliseconds(100));
        huntingTimer->async_wait(bind(&Pathfinder::HuntingKing, this));
        apiWrapper.SubscribeSymbolReady(bind(&Pathfinder::symbolReadyHandler, this, placeholders::_1));
	}

	Pathfinder::~Pathfinder()
	{
	}

	void Pathfinder::symbolReadyHandler(map<string, HttpWrapper::BinanceSymbolData> &data) {
        // 每秒检测连接情况 todo 需要增加depth有效性检测，有可能连接存在但数据为空
        scanDepthTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService, websocketpp::lib::asio::milliseconds(1000));
        scanDepthTimer->async_wait(bind(&Pathfinder::scanDepthSocket, this));

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
        huntingTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService, websocketpp::lib::asio::milliseconds(100));
        huntingTimer->async_wait(bind(&Pathfinder::HuntingKing, this));

        auto path = Graph::CalculateArbitrage();
        if (path.size() != 3) {
            return;
        }

        vector<string> info;
        info.push_back(path[0].FromToken);
        for (auto item:path) {
            info.push_back(item.ToToken);
        }
        spdlog::info("func: {}, path: {}", "HuntingKing", spdlog::fmt_lib::join(info, ","));

        TransactionPath Path{};
        Path.Path = path;
        this->subscriber(Path);
    }

	void Pathfinder::depthDataHandler(WebsocketWrapper::DepthData &data)
	{
        if (data.Asks.size() > 0) {
            Graph::AddEdge(data.FromToken, data.ToToken, data.Asks[0].Price);
        }
        if (data.Asks.size() > 0) {
            Graph::AddEdge(data.ToToken, data.FromToken, data.Bids[0].Price);
        }
	}

	void Pathfinder::SubscribeArbitrage(function<void(TransactionPath &path)> handler)
	{
		this->subscriber = handler;
	}

	int Pathfinder::RevisePath(RevisePathReq req, TransactionPath &resp)
	{
		// 1. 在负权图中计算路径
		// 2. 返回下一步交易路径
        auto nextStep = Graph::FindBestPath(req.Origin, req.End);

        GetExchangePriceReq priceReq{};
        GetExchangePriceResp priceResp{};
        priceReq.FromToken = req.Origin;
        priceReq.ToToken = nextStep;
        Graph::GetExchangePrice(priceReq, priceResp);

        TransactionPathItem item;
        item.FromToken = req.Origin;
        item.FromPrice = priceResp.FromPrice;
        item.ToToken = nextStep;
        item.ToPrice = priceResp.ToPrice;

        return 0;
    }
}
