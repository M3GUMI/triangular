#include <iostream>
#include <cctype>
#include <algorithm>
#include <sstream>
#include <string>
#include <chrono>
#include <random>
#include <map>
#include "utils.h"

void String2Double(const string &str, double &d)
{
	istringstream s(str);
	s >> d;
}

uint64_t GetNowTime()
{

	timeb t;
	ftime(&t); // 获取毫秒
	time_t curr = t.time * 1000 + t.millitm;
	uint64_t time = ((uint64_t)curr);
	return time;
}

unsigned long getRand()
{
	static default_random_engine e(chrono::system_clock::now().time_since_epoch().count() / chrono::system_clock::period::den);
	return e();
}

string GenerateId()
{
	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch());

	// 获取 long long的OrderId
	unsigned long long timestamp = ms.count();
	unsigned long long longOrderId = (timestamp << 32 | getRand());

	// 转long long 转 string
	stringstream stream;
	string result;

	stream << longOrderId;
	stream >> result;
	return result;
}

string toLower(const string &str)
{
	string s = str;
	transform(s.begin(), s.end(), s.begin(), ::tolower);
	return s;
}

string toUpper(const string &str)
{
	string s = str;
	transform(s.begin(), s.end(), s.begin(), ::toupper);
	return s;
}

void LogDebug(string arg1, string arg2, string arg3, string arg4) {
	vector<string> args;
	args.push_back(arg1);
	args.push_back(arg2);
	args.push_back(arg3);
	args.push_back(arg4);
	log("[Debug]", args);
}

void LogInfo(string arg1, string arg2, string arg3, string arg4) {
	vector<string> args;
	args.push_back(arg1);
	args.push_back(arg2);
	args.push_back(arg3);
	args.push_back(arg4);
	log("[Info]", args);
}

void LogInfo(string arg1, string arg2, string arg3, string arg4, string arg5, string arg6) {
	vector<string> args;
	args.push_back(arg1);
	args.push_back(arg2);
	args.push_back(arg3);
	args.push_back(arg4);
	args.push_back(arg5);
	args.push_back(arg6);
	log("[Info]", args);
}

void LogError(string arg1, string arg2, string arg3, string arg4) {
	vector<string> args;
	args.push_back(arg1);
	args.push_back(arg2);
	args.push_back(arg3);
	args.push_back(arg4);
	log("[Error]", args);
}

void LogError(string arg1, string arg2, string arg3, string arg4, string arg5, string arg6) {
	vector<string> args;
	args.push_back(arg1);
	args.push_back(arg2);
	args.push_back(arg3);
	args.push_back(arg4);
	args.push_back(arg5);
	args.push_back(arg6);
	log("[Error]", args);
}

void log(string level, vector<string> args)
{
	bool first = true;
	bool isVal = false;
	for (auto arg : args)
	{
		if (!isVal)
		{
			if (!first)
			{
				cout << ", ";
			}
			isVal = true;
			cout << arg << "=";
		}
		else
		{
			isVal = false;
			cout << arg;
		}

		first = false;
	}

	cout << endl;
}

string WrapErr(int err)
{
	if (!define::ErrorMsgMap.count(err))
	{
		return "未定义错误";
	}

	return define::ErrorMsgMap[err];
}