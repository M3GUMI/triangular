#include <string>
#include "pathfinder.h"

using namespace std;
namespace Pathfinder
{
	Pathfinder::Pathfinder()
	{
		API::GetBinanceDepthWrapper().SubscribeDepth(bind(&Pathfinder::depthDataHandler, this, placeholders::_1));
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
}
