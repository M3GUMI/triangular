#include <sys/timeb.h>
#include <string>

using namespace std;

pair<string, string> GetExchangeKey();

void String2Double(const string &str, double &d);
uint64_t GetNowTime();
unsigned long getRand();
string GenerateId();