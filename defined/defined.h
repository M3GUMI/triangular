#pragma once
#include <string>

using namespace std;

const string RetStatusSuccess = "SUCCESS";
const string RetStatusFail = "FAIL";
const string RetStatusRetry = "RETRY";

struct BaseResp
{
	string RetStatus;
	string RetCode;
	string RetMsg;
};

BaseResp *NewBaseResp(string msg);
BaseResp *FailBaseResp(string errMsg);

bool IsSucc(BaseResp *base);
bool IsRetry(BaseResp *base);
bool IsFail(BaseResp *base);