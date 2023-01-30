#pragma once
#include <string>
#include <map>
#include <functional>
#include "lib/api/api.h"

using namespace std;
namespace CapitalPool
{
    struct Asset{
        string Token;
        double Amount;
    };

    struct Position
    {
        string PositionId;
        map<string, Asset> AssetMap; // <token,asset>
    };

    class CapitalPool
    {
    private:
        map<string, double> basePool;        // 初始资金池，重平衡至该状态
        map<string, double> balancePool;     // 实际资金池，<token,amount>
        map<string, Position> positionMap; // 持仓，<positionId,position>

        int tryRebalance(std::string to, std::string from, double amount);
        void rebalanceHandler(HttpWrapper::OrderData &data);

    public:
        CapitalPool(vector<Asset> assets);
        ~CapitalPool();

        void RebalancePool(); // 重平衡资金池

        int GetPosition(string id, Position &position);                  // 获取仓位
        int CreatePosition(Asset &asset, Position &position);            // 建仓。资金抽出资金池并锁定
        int UpdatePositionAsset(string id, string token, double amount); // 调整仓位资产。套利收益更新
        int FreePosition(string id, Asset &asset);                       // 释放仓位。资金回到资金池
        int EmptyPosition(string id);                                    // 清仓。资金回到资金池
    };
}