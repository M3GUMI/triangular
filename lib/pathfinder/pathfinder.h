#pragma once
#include <string>
#include <time.h>

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

	/*struct NotifyDepthReq
	{
		std::string FromToken; // 卖出的token
		std::string ToToken; // 买入的token
		time_t UpdateTime; // 更新时间，ms精度
		DepthItem Bids[10];
		DepthItem Asks[10];
	};*/

	typedef void (*PathfinderSubscriberHandler)(TransactionPath *path);

	class Pathfinder
	{
	public:
		Pathfinder();
		~Pathfinder();

		void Init();
		void RevisePath(string origin, string end);
		void SubscribeArbitrage(PathfinderSubscriberHandler handler);

	private:
		PathfinderSubscriberHandler subscriber = NULL;

		void depthDataHandler();
		TransactionPath *getTriangularPath();
	};
}