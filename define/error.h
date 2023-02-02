#pragma once
#include <map>
#include <string>

using namespace std;
namespace define
{
    extern map<int, string> ErrorMsgMap;

    // 1-99 通用错误
    const int ErrorDefault = 1;
    const int ErrorInvalidParam = 2;
    const int ErrorInvalidResp = 3;

    // 100-199, http错误
    const int ErrorHttpFail = 100;
    const int ErrorEmptyResponse = 101;

    const int ErrorInsufficientBalance = 200;
}