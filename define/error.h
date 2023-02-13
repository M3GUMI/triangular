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

    const int ErrorInsufficientBalance = 200; // 账户资金不足
    const int ErrorCapitalRefresh = 201; // 资金池锁定
    const int ErrorCreateWebsocketFail = 202; // 建立websocket连接失败
    const int ErrorBalanceNumber = 203; // 资金池金额计算错误
    const int ErrorGraphNotReady = 204; // token价格图未就绪
    const int ErrorStrategy = 205; // 策略错误
}