#include "conf.h"
#include "utils/utils.h"

namespace conf
{
//    bool EnableMock = true;
//
//    string AccessKey = "c52zdrltx6vSMgojFzxJcVQ1v7qiD55G0PgTe31v3fCfEazqgnBu3xNRWOPVOj86";
//    string SecretKey = "lDOZfpTNBIG8ICteeNfoOIoOHBONvBsiAP88GJ5rgDMF6bGGPETkM1Ri14mrbkfJ";
//
//    double MaxTriangularQuantity = 60;
//    double MinTriangularQuantity = 15;
//
//    string BaseAsset = "BUSD";
//    map<string, double> BaseAssets = {
//        {"BUSD", 30},
//    };
    Config* initConfig()
    {
        readConfig();
        return getConfig();
    }
}
