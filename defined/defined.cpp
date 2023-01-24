#include "defined.h"
#include <string>
#include <sstream>

using namespace std;

BaseResp *NewBaseResp(string msg)
{
	BaseResp base;
	base.RetStatus = RetStatusSuccess;
	base.RetCode = "";
	base.RetMsg = msg;

	return &base;
};

BaseResp *FailBaseResp(string errMsg)
{
	BaseResp base;
	base.RetStatus = RetStatusFail;
	base.RetCode = "";
	base.RetMsg = errMsg;

	return &base;
};

bool IsSucc(BaseResp *base)
{
	if (base == NULL)
	{
		return false;
	};

	return base->RetStatus == RetStatusSuccess;
}

bool IsRetry(BaseResp *base)
{
	if (base == NULL)
	{
		return false;
	};

	return base->RetStatus == RetStatusRetry;
}

bool IsFail(BaseResp *base)
{
	if (base == NULL)
	{
		return true;
	};

	return base->RetStatus == RetStatusFail;
}
