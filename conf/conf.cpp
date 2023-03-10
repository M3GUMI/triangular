#include "conf.h"
#include "utils/utils.h"

namespace conf
{
    bool EnableMock = true;

    string AccessKey = "";
    string SecretKey = "";

    double MaxTriangularQuantity = 60;
    double MinTriangularQuantity = 15;

    string BaseAsset = "BUSD";
    map<string, double> BaseAssets = {
        {"BUSD", 30},
    };
    map<u_int64_t, double> HandlingFeeMap = {
            {formatKey(define::LIMIT_MAKER, define::GTC), 0},
            {formatKey(define::MARKET, define::IOC), 0.0004},
    };
//    Config* initConfig()
//    {
//        readConfig();
//        return getConfig();
//    }
    u_int64_t formatKey(int OrderType, int TimeInForce)
    {
        u_int64_t result = u_int64_t(OrderType) << 32 | TimeInForce;
        return result;
    }
}
