#include "conf.h"

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
}