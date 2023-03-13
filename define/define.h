
#pragma once
#include <string>
#include "error.h"

using namespace std;
namespace define
{
    bool IsStableCoin(string coinName);
    bool NotStableCoin(string coinName);

    enum SocketStatus
    {
        SocketStatusInit = 0,
        SocketStatusConnected = 1,
        SocketStatusFailConnect = 2
    };

    enum OrderSide
    {
        INVALID_SIDE = 0,
        BUY = 1,
        SELL = 2
    };

    enum OrderType
    {
        LIMIT = 1,             // 限价单
        MARKET = 2,            // 市价单
        STOP_LOSS = 3,         // 止损单
        STOP_LOSS_LIMIT = 4,   // 限价止损单
        TAKE_PROFIT = 5,       // 止盈单
        TAKE_PROFIT_LIMIT = 6, // 限价止盈单
        LIMIT_MAKER = 7        // 限价只挂单
    };

    enum TimeInForce
    {
        GTC = 1, // 成交为止
        IOC = 2, // 无法立即成交的部分就撤销
        FOK = 3  // 无法全部立即成交就撤销
    };

    enum OrderStatus
    {
        INVALID_ORDER_STATUS = 0, // 非法订单状态
        INIT = 1,                 // 本地系统初始化，未提交
        NEW = 2,                  // 订单被交易引擎接受
        PARTIALLY_FILLED = 3,     // 部分订单被成交
        FILLED = 4,               // 订单完全成交
        CANCELED = 5,             // 用户撤销了订单
        PENDING_CANCEL = 6,       // 撤销中(目前并未使用)
        REJECTED = 7,             // 订单没有被交易引擎接受，也没被处理
        EXPIRED = 8,              // 订单被交易引擎取消, 比如
    };
}