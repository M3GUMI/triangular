#pragma once
#include <string>
#include <map>
#include "lib/api/http/binance_api_wrapper.h"
#include "lib/pathfinder/pathfinder.h"

using namespace std;
namespace CapitalPool
{
    class CapitalPool
    {
    private:
        HttpWrapper::BinanceApiWrapper &apiWrapper;
        Pathfinder::Pathfinder &pathfinder;

        map<string, double> basePool;    // 初始资金池，重平衡至该状态
        map<string, double> balancePool; // 实际资金池，<token,amount>

        bool locked; // 刷新中，锁定
        int tryRebalance(std::string to, std::string from, double amount);
        void rebalanceHandler(HttpWrapper::OrderData &data);
        void refreshAccountHandler(HttpWrapper::AccountInfo &info, int err);

    public:
        CapitalPool(Pathfinder::Pathfinder pathfinder, HttpWrapper::BinanceApiWrapper api);
        ~CapitalPool();

        void RebalancePool(); // 重平衡资金池

        int LockAsset(string token, double amount); // 资金抽出资金池
        int FreeAsset(string token, double amount); // 释放资金回到资金池
        int Refresh();                              // 清空所有订单，重加载资金池
    };
}