#pragma once
#include "maker_triangular.h"
#include "lib/order/order.h"
#include "lib/pathfinder/node.h"
#include "lib/pathfinder/pathfinder.h"
#include "define/define.h"
#include "conf/strategy.h"


using namespace std;
using namespace Arbitrage;

namespace makerMock
{
    class MakerMock {

    public:

    protected:

        MakerMock(
                Pathfinder::Pathfinder &pathfinder,
                Arbitrage::TriangularArbitrage triangular
                );
        ~MakerMock();

        /*TODO
         *  1 tradeNodeHandler从tradeNodeMap 获取现在的价格，回调mockTrader
            2 mockTrader从ordermap取出订单数据进行比较，若满足则修改订单状态并装配成交数据，并调用baseorderhandler
            3 定时30s没能完成订单交易则利用阈值修改tradeNode，并回调traderNodeHander!
        */
        map<u_int64_t, Pathfinder::Node*> mockTradeNodeMap{}; // key为baseIndex+quoteIndex、quoteIndex+baseIndex两种类型 存放tradeNodeMap
        map<uint64_t, OrderData*> mockOrderMap;
        Arbitrage::TriangularArbitrage &triangular;
        Pathfinder::Pathfinder &pathfinder;
        void tradeNodeHandler(map<u_int64_t, Pathfinder::Node*> tradeNodeMap);
        void mockTrader(map<u_int64_t, Pathfinder::Node*> tradeNodeMap, map<uint64_t, OrderData*> mockOrderMap);
        void initMock();
    private:

    };
}