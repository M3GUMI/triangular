#pragma once
#include <string>
#include <map>
#include <set>
#include "lib/pathfinder/pathfinder.h"
#include "lib/capital_pool/capital_pool.h"
#include "lib/api/http/binance_api_wrapper.h"
#include "lib/order/order.h"
#include "define/define.h"

using namespace std;
namespace Arbitrage{
    // 三角套利
    class TriangularArbitrage {
    public:
        virtual int Run(Pathfinder::ArbitrageChance &chance);
        void SubscribeFinish(function<void()> callback);

    protected:
        TriangularArbitrage(
                Pathfinder::Pathfinder &pathfinder,
                CapitalPool::CapitalPool &pool,
                HttpWrapper::BinanceApiWrapper &apiWrapper
        );
        ~TriangularArbitrage();

        map<uint64_t, OrderData*> orderMap; // 订单map

        string strategy;           // 策略名
        string TargetToken;        // 目标token
        double OriginQuantity = 0; // 原始起点token数量
        double FinalQuantity = 0; // 最终起点token数量

        Pathfinder::Pathfinder &pathfinder;
        CapitalPool::CapitalPool &capitalPool;
        HttpWrapper::BinanceApiWrapper &apiWrapper;

        int ExecuteTrans(Pathfinder::TransactionPathItem &path);
        int ReviseTrans(string origin, string end, double quantity);
        bool CheckFinish();

    private:
        bool finished = false;
        function<void()> subscriber = nullptr;

        virtual void TransHandler(OrderData &orderData);
        void baseOrderHandler(OrderData &data, int err);
    };
}