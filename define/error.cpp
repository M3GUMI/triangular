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
        {ErrorBalanceNumber, "number error balance_pool"},
        {ErrorGraphNotReady, "graph price not ready"},
        {ErrorStrategy, "transaction strategy error"},
        {ErrorOrderAmountZero, "order amount is zero"},
        {ErrorLessTicketSize, "less than ticket_size"},
    };
}