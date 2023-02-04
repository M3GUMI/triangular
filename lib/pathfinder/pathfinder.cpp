#include "utils/utils.h"
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

	void Pathfinder::symbolReadyHandler(map<string, HttpWrapper::BinanceSymbolData> &data)
	{
		LogDebug("func", "LoadDepth", "msg", "init depth websocket");

		auto succNum = 0, failNum = 0;
		for (auto item : data)
		{
			auto symbol = item.first;
			auto symbolData = item.second;

			if (symbol == symbolData.BaseToken + symbolData.QuoteToken)
			{

				// todo 补一个连接管理，内存释放+断连重试
				auto *depthWrapper = new WebsocketWrapper::BinanceDepthWrapper(ioService, apiWrapper, "stream.binance.com", "9443");
				if (auto err = (*depthWrapper).Connect(symbolData.BaseToken, symbolData.QuoteToken); err > 0)
				{
					failNum++;
				}
				else
				{
					(*depthWrapper).SubscribeDepth(bind(&Pathfinder::depthDataHandler, this, placeholders::_1));
					succNum++;
				}
			}
		}

		LogInfo("func", "LoadDepth", "success_num", to_string(succNum - failNum), "fail_num", to_string(failNum));
	}

	void Pathfinder::MockRun()
	{
		TransactionPathItem item;
		vector<TransactionPathItem> Path;
		Path.push_back(item);

		TransactionPath data;
		data.Path = Path;

		subscriber(data);
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
		resp.FromPrice = 0;
		resp.ToPrice = 0;
		return 0;
	}
}
