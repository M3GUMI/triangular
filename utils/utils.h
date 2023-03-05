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

double String2Double(const string &str);

extern uint32_t autoIncr; // 自增id
uint64_t GenerateId();
uint64_t GetNowTime();

string toLower(const string &str);
string toUpper(const string &str);

string WrapErr(int err);
string FormatDouble(double val); // 数量、价格转8位小数字符串
double RoundDouble(double val);
double Min(double val0, double val1);
