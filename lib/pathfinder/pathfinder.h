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
	struct TransactiItemonPath
	{
		std::string FromToken; // 卖出的token
		double FromPrice;	       // 卖出价格
		double FromQuantity;       // 卖出数量
		std::string ToToken;   // 买入的token
		double ToPrice;	//买入价格
		double ToQuantity; //买入数量
	};

	struct TransactionPath
	{
		TransactiItemonPath Path[10];
	};

	/*struct NotifyDepthReq
	{
		std::string FromToken; // 卖出的token
		std::string ToToken; // 买入的token
		time_t UpdateTime; // 更新时间，ms精度
		DepthItem Bids[10];
		DepthItem Asks[10];
	};*/

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