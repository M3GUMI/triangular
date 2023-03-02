#include <iomanip>
#include <iostream>
#include <cctype>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <string>
#include <chrono>
#include "utils.h"
#include <fstream>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

void String2Double(const string &str, double &d) {
    istringstream s(str);
    s >> d;
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

double FormatDoubleV2(double val) {
    double result = round(val*pow(10, 8)) / pow(10, 8);
    return result;
}
//
//void readConfig(){
//    // 读取文件内容
//    std::ifstream ifs("config.json");
//    std::string content((std::istreambuf_iterator<char>(ifs)),(std::istreambuf_iterator<char>()));
//
//    // 解析JSON
//    rapidjson::Document doc;
//    doc.Parse(content.c_str());
//
//    // 获取配置项
//    bool EnableMock = doc["EnableMock"].GetBool();
//    std::string AccessKey = doc["AccessKey"].GetString();
//    std::string SecretKey = doc["SecretKey"].GetString();
//    double MaxTriangularQuantity = doc["MaxTriangularQuantity"].GetDouble();
//    double MinTriangularQuantity = doc["MinTriangularQuantity"].GetDouble();
//    std::string BaseAsset = doc["BaseAsset"].GetString();
//    for (rapidjson::Value::ConstMemberIterator it = doc["BaseAssets"].MemberBegin(); it != doc["BaseAssets"].MemberEnd(); ++it) {
//        string key = it->name.GetString();
//        double value = it->value.GetDouble();
//        config.BaseAssets[key] = value;
//    }
//    config.AccessKey = AccessKey;
//    config.BaseAsset = BaseAsset;
//    config.MinTriangularQuantity = MinTriangularQuantity;
//    config.MaxTriangularQuantity = MaxTriangularQuantity;
//    config.EnableMock = EnableMock;
//    config.SecretKey = SecretKey;
//
//}

//Config* getConfig(){
//    return &config;
//}
