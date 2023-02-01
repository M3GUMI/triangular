#pragma once
#include <map>
#include <string>

using namespace std;
namespace define
{
    const int ErrorDefault = 1;

    // 100-199, http错误
    const int ErrorHttpFail = 100;  
    const int ErrorEmptyResponse = 101;

    // 200-299 通用错误
    const int ErrorInvalidParam = 200;
    const int ErrorInvalidResp = 201;

    const int ErrorInsufficientBalance = 200;

    string WrapErr(int err);
}