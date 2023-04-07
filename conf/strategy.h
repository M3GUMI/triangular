#pragma once
#include <vector>
#include "define/define.h"
#include "conf/conf.h"

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
            if (phase == 0) {
                return Steps[0];
            }

            return Steps[phase-1];
        }

        double GetFee(int phase)
        {
            if (phase == 0) {
                phase = 1;
            }

            double fee = 1;
            for (int i = phase - 1; i < Steps.size(); i++)
            {
                auto step = Steps[i];
                fee = fee*(1 - HandlingFeeMap[formatKey(step.OrderType, step.TimeInForce)]);
            }

            return fee;
        }
    };

    extern Strategy MakerTriangular;
    extern Strategy IOCTriangular;
}