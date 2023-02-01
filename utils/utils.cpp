#include <sstream>
#include <string>
#include <chrono>
#include <random>
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

pair<string, string> GetExchangeKey()
{
	return make_pair("access", "secret");
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