#pragma once
#include <string>

using namespace std;
namespace conf
{
    extern bool EnableMock; // 阻断需要key的http调用，并mock回调函数
    extern string AccessKey;
    extern string SecretKey;
}