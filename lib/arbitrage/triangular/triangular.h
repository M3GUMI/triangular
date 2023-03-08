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
        virtual int Run(Pathfinder::ArbitrageChance &chance);
        void SubscribeFinish(function<void()> callback);

        void baseOrderHandler(OrderData &data, int err);

    protected:
        TriangularArbitrage(
                Pathfinder::Pathfinder &pathfinder,
                CapitalPool::CapitalPool &pool,
                HttpWrapper::BinanceApiWrapper &apiWrapper
        );
        ~TriangularArbitrage();

        map<uint64_t, OrderData*> orderMap; // 订单map

        string strategy;           // 策略名
        conf::Strategy strategyV2; // 策略名
        string TargetToken;        // 目标token
        double OriginQuantity = 0; // 原始起点token数量
        double FinalQuantity = 0;  // 最终起点token数量

        Pathfinder::Pathfinder &pathfinder;
        CapitalPool::CapitalPool &capitalPool;
        HttpWrapper::BinanceApiWrapper &apiWrapper;

        int ExecuteTrans(int phase, Pathfinder::TransactionPathItem &path);
        int ReviseTrans(string origin, string end, int phase, double quantity);
        bool CheckFinish();

        int executeOrder(OrderData &orderData);//挂出新订单
        int cancelOrder(OrderData &orderData);//取消订单
    private:
        bool finished = false;
        function<void()> subscriber = nullptr;
        virtual void TransHandler(OrderData &orderData);
    };
}