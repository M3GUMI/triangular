#include "error.h"

namespace define
{
    map<int, string> ErrorMsgMap = {
        // 1-99 通用错误
        {ErrorDefault, "unknown error"},
        {ErrorInvalidParam, "invalid param"},
        {ErrorInvalidResp, "invalid resp"},

        // 100-199 http错误
        {ErrorHttpFail, "http fail"},
        {ErrorEmptyResponse, "http resp empty"},

        {ErrorCreateWebsocketFail, "create websocket client fail"},
        {ErrorInsufficientBalance, "account balance insufficient"},
        {ErrorCapitalRefresh, "refreshing capital_pool"},
    };
}