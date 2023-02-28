#include "conf.h"

namespace conf
{
    bool EnableMock = true;

    string AccessKey = "bBDPMyhkBrKm3u6KrRtCD2F5jIJ1EREvmpwN62aV8T27oMFvwPTGUoo3MtEU7SCO";
    string SecretKey = "R38810tJ27f8ua8cvoGXV1yzOMMaxTXicV74IjGuU9rp1HeCB8DsXu5XolVMAp9T";

    double MaxTriangularQuantity = 60;
    double MinTriangularQuantity = 15;

    string BaseAsset = "BUSD";
    map<string, double> BaseAssets = {
        {"BUSD", 30},
    };
}