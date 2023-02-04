#include <string>
#include "pathfinder.h"

using namespace std;
namespace Pathfinder
{
    Pathfinder* pathfinder;

    void Init() {
		auto data = Pathfinder();
		pathfinder = &data;
	}

	Pathfinder& GetPathfinder() {
		if (pathfinder == NULL) {
			Init();
		}

		return *pathfinder;
	}

	Pathfinder::Pathfinder()
	{
		API::GetBinanceDepthWrapper().SubscribeDepth(bind(&Pathfinder::depthDataHandler, this, placeholders::_1));
	}

	Pathfinder::~Pathfinder()
	{
	}

	void Pathfinder::depthDataHandler(WebsocketWrapper::DepthData& data)
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
