#include <sys/timeb.h>
#include <string>
#include <vector>
#include "define/define.h"

using namespace std;
using namespace define;

pair<string, string> GetExchangeKey();

void String2Double(const string &str, double &d);
uint64_t GetNowTime();
unsigned long getRand();
string GenerateId();

void log(string level, vector<string> args);
void LogDebug(string arg1, string arg2, string arg3, string arg4);
void LogInfo(string arg1, string arg2, string arg3, string arg4);
void LogError(string arg1, string arg2, string arg3, string arg4);
