#pragma once
#include <string>
#include <time.h>
#include <functional>

using namespace std;
namespace Pathfinder
{
	// 深度数据
	struct DepthItem
	{
		double Price; // 价格
		double Quantity; // 数量
	};

	// 套利路径
	struct TransactionPathItem
	{
		std::string FromToken; // 卖出的token
		std::string ToToken;   // 买入的token
		double Price;	       // 参考价格
		double Quantity;       // 参考数量
	};

	struct TransactionPath
	{
		TransactionPathItem Path[10];
	};

	typedef function<void(TransactionPath *path)> PathfinderSubscriberHandler;

	class Pathfinder
	{
	public:
		void RevisePath(string origin, string end);
		void SubscribeArbitrage(PathfinderSubscriberHandler handler);

	private:
		PathfinderSubscriberHandler subscriber = NULL;

		void depthDataHandler();
		TransactionPath *getTriangularPath();
	};
}