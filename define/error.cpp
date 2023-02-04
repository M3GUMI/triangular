#include "error.h"

namespace define
{
    map<int, string> ErrorMsgMap = {
        // 1-99 通用错误
        {ErrorDefault, "默认错误"},
        {ErrorInvalidParam, "参数非法"},
        {ErrorInvalidResp, "返回参数非法"},

        // 100-199 http错误
        {ErrorHttpFail, "http请求失败"},
        {ErrorEmptyResponse, "返回数据为空"},

        {ErrorInsufficientBalance, "账户余额不足"},
        {ErrorCapitalRefresh, "资金池刷新中"},
    };
}