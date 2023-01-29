#include "define.h"

namespace define
{
    bool IsStableCoin(string coinName)
    {
        if (coinName == "ETH")
        {
            return true;
        }
        else if (coinName == "USDT")
        {
            return true;
        }
        else if (coinName == "BTC")
        {
            return true;
        }

        return false;
    }

    bool NotStableCoin(string coinName)
    {
        return !IsStableCoin(coinName);
    }
}