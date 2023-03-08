#pragma once
#include <vector>
#include "define/define.h"

namespace conf
{
    struct Step
    {
        bool StableCoin = false; // 是否要求稳定币
        define::OrderType OrderType = define::LIMIT; // 订单类型
        define::TimeInForce TimeInForce = define::IOC; // 执行类型
    };

    struct Strategy
    {
        string StrategyName; // 策略名
        vector<Step> Steps; // 执行步骤

        Step& GetStep(int phase) {
            return Steps[phase-1];
        }
    };

    extern Strategy MakerTriangular;
}