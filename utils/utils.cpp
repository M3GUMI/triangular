#include <sstream>
#include <string>
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

pair<string, string> GetAccessKey()
{
	return make_pair("access", "secret");
}