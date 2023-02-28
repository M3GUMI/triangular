#pragma once
#include <sys/timeb.h>
#include <string>
#include <vector>
#include <set>
#include "define/define.h"
#include "conf/conf.h"
#include "spdlog/spdlog.h"
#include "spdlog/formatter.h"

using namespace std;
using namespace define;


struct Config{
    bool EnableMock;    // 阻断需要key的http调用，并mock回调函数

    string AccessKey;
    string SecretKey;

     double MaxTriangularQuantity;
     double MinTriangularQuantity;

    string BaseAsset; // 最终稳定币
     map<string, double> BaseAssets; // 初始仓位
}config;
void String2Double(const string &str, double &d);

extern uint32_t autoIncr; // 自增id
uint64_t GenerateId();
uint64_t GetNowTime();

string toLower(const string &str);
string toUpper(const string &str);

string WrapErr(int err);
string FormatDouble(double val); // 数量、价格转8位小数字符串
double FormatDoubleV2(double val);

void readConfig();

Config* getConfig();
