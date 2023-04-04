#pragma once
#include <map>
#include <string>
#include <utils/utils.h>
#include "define/define.h"
#include <cmath>

using namespace std;
namespace conf
{
    extern bool EnableMock;    // 阻断需要key的http调用，并mock回调函数
    extern bool IsIOC;

    extern string AccessKey;
    extern string SecretKey;

    extern double MaxTriangularQuantity;
    extern double MinTriangularQuantity;

    extern string BaseAsset; // 最终稳定币
    extern map<string, double> BaseAssets; // 初始仓位
    extern map<u_int64_t, double> HandlingFeeMap;

    u_int64_t formatKey(int OrderType, int TimeInForce);

//    Config* initConfig();
}