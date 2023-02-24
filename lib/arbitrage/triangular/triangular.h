#pragma once
#include <string>
#include <map>
#include <set>
#include "lib/pathfinder/pathfinder.h"
#include "lib/capital_pool/capital_pool.h"
#include "lib/api/http/binance_api_wrapper.h"
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

        map<uint64_t, HttpWrapper::OrderData*> orderMap; // 订单map

        string TargetToken;       // 目标token
        double OriginQuantity = 0; // 原始起点token数量

        Pathfinder::Pathfinder &pathfinder;
        CapitalPool::CapitalPool &capitalPool;
        HttpWrapper::BinanceApiWrapper &apiWrapper;

        int ExecuteTrans(Pathfinder::TransactionPathItem &path);
        int ReviseTrans(string origin, string end, double quantity);
        bool CheckFinish();
        int Finish(double finalQuantity);

    private:
        function<void()> subscriber = nullptr;

        virtual void TransHandler(HttpWrapper::OrderData &orderData);
        void baseOrderHandler(HttpWrapper::OrderData &data, int err);
    };
}