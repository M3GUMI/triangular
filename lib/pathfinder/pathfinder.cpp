#include "utils/utils.h"
#include "conf/conf.h"
#include "pathfinder.h"

using namespace std;
namespace Pathfinder
{
	Pathfinder::Pathfinder(websocketpp::lib::asio::io_service &ioService, HttpWrapper::BinanceApiWrapper &apiWrapper) : ioService(ioService), apiWrapper(apiWrapper)
	{
		if (conf::EnableMockRun) {
			this->loadMockPath();
		}
		apiWrapper.SubscribeSymbolReady(bind(&Pathfinder::symbolReadyHandler, this, placeholders::_1));
	}

	Pathfinder::~Pathfinder()
	{
	}

	void Pathfinder::symbolReadyHandler(map<string, HttpWrapper::BinanceSymbolData> &data)
	{
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

		LogInfo("func", "symbolReadyHandler", "init_num", to_string(succNum - failNum), "fail_num", to_string(failNum));
	}

	void Pathfinder::loadMockPath()
	{
		TransactionPathItem pathItem1;
		pathItem1.FromToken = "USDT";
		pathItem1.FromPrice = double(1) / double(1258);
		pathItem1.FromQuantity = 1258;
		pathItem1.ToToken = "ETH";
		pathItem1.ToPrice = 1258;
		pathItem1.ToQuantity = 1;

		TransactionPathItem pathItem2;
		pathItem2.FromToken = "ETH";
		pathItem2.FromPrice = 1259;
		pathItem2.FromQuantity = 1;
		pathItem2.ToToken = "BUSD";
		pathItem2.ToPrice = double(1) / double(1259);
		pathItem2.ToQuantity = 1259;

		TransactionPathItem pathItem3;
		pathItem3.FromToken = "BUSD";
		pathItem3.FromPrice = 1;
		pathItem3.FromQuantity = 1259;
		pathItem3.ToToken = "USDT";
		pathItem3.ToPrice = 1258;
		pathItem3.ToQuantity = double(1259) / double(1258);

		this->mockPath.Path.push_back(pathItem1);
		this->mockPath.Path.push_back(pathItem2);
		this->mockPath.Path.push_back(pathItem3);
	}

	void Pathfinder::depthDataHandler(WebsocketWrapper::DepthData &data)
	{
		if (conf::EnableMockRun)
		{
			return this->subscriber(this->mockPath);
		}
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
		if (conf::EnableMockRun)
		{
			mockPath.Path.erase(mockPath.Path.begin());
			for (int i = 0; i < mockPath.Path.size(); i++)
			{
				resp.Path.push_back(mockPath.Path[i]);
			}
			return 0;
		}
	}

	int Pathfinder::GetExchangePrice(GetExchangePriceReq &req, GetExchangePriceResp &resp)
	{
		// 获取负权图中价格
		resp.FromPrice = 1;
		resp.ToPrice = 1;
		return 0;
	}
}
