#include "conf.h"

namespace conf
{
    bool EnableMock = true;

    string AccessKey = "c52zdrltx6vSMgojFzxJcVQ1v7qiD55G0PgTe31v3fCfEazqgnBu3xNRWOPVOj86";
    string SecretKey = "lDOZfpTNBIG8ICteeNfoOIoOHBONvBsiAP88GJ5rgDMF6bGGPETkM1Ri14mrbkfJ";

    map<string, double> BaseAssets = {
        {"USDT", 200},
        {"BUSD", 0.2},
        {"BUSD", 100},
    };
}