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
        spdlog::debug("func: {}, init: {}, connected: {}, reconnect: {}", "scanDepthSocket", initNum, succNum, failNum);
    }

	void Pathfinder::depthDataHandler(WebsocketWrapper::DepthData &data)
	{
		// 1. 更新负权图
		// 2. 触发套利路径计算（如果在执行中，则不触发）
		//     a. 拷贝临时负权图
		//     b. 计算路径
		//     c. 通知交易机会,若存在
		// if this->subscriber != NULL
		// this->subscriber(this->getTriangularPath());
	}

	void Pathfinder::SubscribeArbitrage(function<void(TransactionPath &path)> handler)
	{
		this->subscriber = handler;
	}

	int Pathfinder::RevisePath(RevisePathReq req, TransactionPath &resp)
	{
		// 1. 在负权图中计算路径
		// 2. 返回下一步交易路径
	}

	int Pathfinder::GetExchangePrice(GetExchangePriceReq &req, GetExchangePriceResp &resp)
	{
		// 获取负权图中价格
		resp.FromPrice = 1;
		resp.ToPrice = 1;
		return 0;
	}
}
