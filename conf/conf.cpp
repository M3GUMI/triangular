#include "conf.h"

namespace conf
{
    bool EnableMock = true;
    bool EnableMockRun = false;

    string AccessKey = "c52zdrltx6vSMgojFzxJcVQ1v7qiD55G0PgTe31v3fCfEazqgnBu3xNRWOPVOj86";
    string SecretKey = "lDOZfpTNBIG8ICteeNfoOIoOHBONvBsiAP88GJ5rgDMF6bGGPETkM1Ri14mrbkfJ";

    map<string, double> BaseAssets = {
        {"USDT", 200},
        {"ETH", 1},
        {"BUSD", 100},
    };
}