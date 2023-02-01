#include "error.h"

namespace define
{

    string WrapErr(int err)
    {
        map<int, string> ErrorMsgMap = {
            // 100-199 http错误
            {ErrorHttpFail, "http请求失败"},
            {ErrorEmptyResponse, "返回数据为空"},

            // 200-299 通用错误
            {ErrorInvalidParam, "参数非法"},
            {ErrorInvalidResp, "返回参数非法"}
        };

        return ErrorMsgMap[err];
    }
}