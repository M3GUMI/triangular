#pragma once
#include <map>
#include <string>

using namespace std;
namespace conf
{
    extern bool EnableMock;    // 阻断需要key的http调用，并mock回调函数
    extern bool EnableMockRun; // 执行mock路径

    extern int LogLevel; // 0：null, 1：error: 2: info, 3: debug
    extern string AccessKey;
    extern string SecretKey;

    extern map<string, double> BaseAssets; // 初始仓位
}