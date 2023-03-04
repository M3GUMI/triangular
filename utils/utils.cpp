#include <iomanip>
#include <iostream>
#include <cctype>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <string>
#include <chrono>
#include "utils.h"

double String2Double(const string &str) {
    double d;
    istringstream s(str);
    s >> d;

    return d;
}

uint64_t GetNowTime() {
    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
    );

    return ms.count();
}

uint32_t autoIncr = 0; // 自增id

uint64_t GenerateId() {
    if (autoIncr == UINT32_MAX) {
        autoIncr = 0;
    }

    autoIncr++;
    return GetNowTime() << 32 | (autoIncr & 0xFFFFFFFF);
}

string toLower(const string &str) {
    string s = str;
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

string toUpper(const string &str) {
    string s = str;
    transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}

string WrapErr(int err) {
    if (!define::ErrorMsgMap.count(err)) {
        spdlog::error("func: WrapErr, msg: 未定义错误, err: {}", err);
        return "未定义错误";
    }

    return define::ErrorMsgMap[err];
}

string FormatDouble(double val) {
    std::stringstream stream;
    stream << std::fixed << std::setprecision(8) << val;

    return stream.str();
}

double RoundDouble(double val) {
    double result = round(val*pow(10, 8)) / pow(10, 8);
    return result;
}