#include <sys/timeb.h>
#include <string>
#include <vector>
#include "define/define.h"
#include "conf/conf.h"
#include "spdlog/spdlog.h"

using namespace std;
using namespace define;

void String2Double(const string &str, double &d);

uint64_t GetNowTime();
unsigned long getRand();
string GenerateId();

string toLower(const string &str);
string toUpper(const string &str);

string WrapErr(int err);