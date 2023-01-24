#pragma once

namespace commondef
{
    enum OrderSide
    {
        UNKNOWN = 0,
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

    enum TimeInFoce
    {
        GTC = 1, // 成交为止
        IOC = 2, // 无法立即成交的部分就撤销
        FOK = 3  // 无法全部立即成交就撤销
    };

    enum OrderStatus
    {
        NEW = 1,              // 订单被交易引擎接受
        PARTIALLY_FILLED = 2, // 部分订单被成交
        FILLED = 3,           // 订单完全成交
        CANCELED = 4,         // 用户撤销了订单
        PENDING_CANCEL = 5,   // 撤销中(目前并未使用)
        REJECTED = 6,         // 订单没有被交易引擎接受，也没被处理
        EXPIRED = 7,          // 订单被交易引擎取消, 比如
    };
}