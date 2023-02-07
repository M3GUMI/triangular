#include <sys/timeb.h>
#include <string>
#include <vector>
#include "define/define.h"

using namespace std;
using namespace define;

void String2Double(const string &str, double &d);

uint64_t GetNowTime();
unsigned long getRand();
string GenerateId();

string toLower(const string &str);
string toUpper(const string &str);

string WrapErr(int err);
void LogDebug(string arg1, string arg2, string arg3, string arg4);
void LogDebug(string arg1, string arg2, string arg3, string arg4, string arg5, string arg6, string arg7, string arg8);
void LogInfo(string arg1, string arg2, string arg3, string arg4);
void LogInfo(string arg1, string arg2, string arg3, string arg4, string arg5, string arg6);
void LogError(string arg1, string arg2, string arg3, string arg4);
void LogError(string arg1, string arg2, string arg3, string arg4, string arg5, string arg6);
void log(string level, vector<string> args);