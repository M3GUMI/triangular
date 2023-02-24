#pragma once
#include <map>
#include <string>

using namespace std;
namespace conf
{
    extern bool EnableMock;    // 阻断需要key的http调用，并mock回调函数

    extern string AccessKey;
    extern string SecretKey;

    extern double MaxTriangularQuantity;
    extern double MinTriangularQuantity;

    extern string BaseAsset; // 最终稳定币
    extern map<string, double> BaseAssets; // 初始仓位
}