#pragma once
#include <string>
#include <map>
#include <set>
#include "lib/pathfinder/pathfinder.h"
#include "lib/capital_pool/capital_pool.h"
#include "lib/api/http/binance_api_wrapper.h"
#include "define/define.h"

using namespace std;
namespace Arbitrage {
    // 三角套利
    class TriangularArbitrage {
    public:
        void SubscribeFinish(function<void()> callback);

    protected:
        TriangularArbitrage(Pathfinder::Pathfinder &pathfinder, CapitalPool::CapitalPool &pool,
                            HttpWrapper::BinanceApiWrapper &apiWrapper);

        ~TriangularArbitrage();

        map<string, HttpWrapper::OrderData> orderMap; // 订单map
        function<void(HttpWrapper::OrderData &data)> transHandler; // 交易策略

        string OriginToken;       // 原始起点token
        double OriginQuantity; // 原始起点token数量
        string TargetToken;       // 目标token

        Pathfinder::Pathfinder &pathfinder;
        CapitalPool::CapitalPool &capitalPool;
        HttpWrapper::BinanceApiWrapper &apiWrapper;

        int ExecuteTrans(Pathfinder::TransactionPathItem &path);

        int ReviseTrans(string origin, string end, double quantity);

        int Finish(double finalQuantity);

    private:
        function<void()> subscriber = nullptr;

        void baseOrderHandler(HttpWrapper::OrderData &data, int err);
    };
}