#pragma once
#include <string>
#include <map>
#include <set>
#include "lib/pathfinder/pathfinder.h"
#include "lib/capital_pool/capital_pool.h"
#include "lib/api/http/binance_api_wrapper.h"
#include "lib/api/ws/binance_depth_wrapper.h"
#include "lib/order/order.h"
#include "define/define.h"
#include "conf/strategy.h"

using namespace std;
namespace Arbitrage{
    // 三角套利
    class TriangularArbitrage {
    public:
        void SubscribeFinish(function<void()> callback);

        void baseOrderHandler(OrderData &data, int err);

        int Run(Pathfinder::ArbitrageChance& chance);

        map<uint64_t, OrderData*> getOrderMap();
        void setOrderMap(map<uint64_t, OrderData*> newOrderMap);
    protected:
        TriangularArbitrage(
                Pathfinder::Pathfinder &pathfinder,
                CapitalPool::CapitalPool &pool,
                HttpWrapper::BinanceApiWrapper &apiWrapper
        );
        ~TriangularArbitrage();

        map<uint64_t, OrderData*> orderMap; // 订单map

        conf::Strategy strategy;   // 策略
        string TargetToken;        // 目标token
        double OriginQuantity = 0; // 原始起点token数量
        double FinalQuantity = 0;  // 最终起点token数量
        double PathQuantity = 0;  //过程币的数量
        bool quitAndReopen = false; //taker次数过多，临时变量判断是否重开
        bool MakerExecuted = false; //判斷maker是否完成成交
        Pathfinder::Pathfinder &pathfinder;
        CapitalPool::CapitalPool &capitalPool;
        HttpWrapper::BinanceApiWrapper &apiWrapper;

        int ExecuteTrans(uint64_t& orderId, int phase, uint64_t cancelOrderId, Pathfinder::TransactionPathItem &path);
        int ReviseTrans(uint64_t& orderId, int phase, string origin, double quantity);
        bool CheckFinish();
        bool finished = false;
    private:
        function<void()> subscriber = nullptr;
        virtual void TransHandler(OrderData &orderData);
        static bool orderStatusCheck(int baseStatus, int newStatus);
    };
}