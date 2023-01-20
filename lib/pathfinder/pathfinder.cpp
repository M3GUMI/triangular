#include <string>
#include "pathfinder.h"

using namespace std;
namespace Pathfinder
{
	void Pathfinder::Init()
	{

	}

	void Pathfinder::SubscribeArbitrage(PathfinderSubscriberHandler handler)
	{
		this->subscriber = handler;
	}

	void Pathfinder::depthDataHandler()
	{
		// 1. 更新负权图
		// 2. 触发套利路径计算（如果在执行中，则不触发）
		//     a. 拷贝临时负权图
		//     b. 计算路径
		//     c. 通知交易机会,若存在
		this->subscriber(this->getTriangularPath());
	}

	void Pathfinder::RevisePath(string origin, string end)
	{
		// 1. 在临时负权图中计算路径
		// 2. 返回下一步交易路径
	}

	TransactionPath *Pathfinder::getTriangularPath()
	{
		TransactionPath req;
		return &req;
	}
}
