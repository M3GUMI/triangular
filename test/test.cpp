#include "lib/triangular/triangular.h"

using namespace std;
int main()
{
    Triangular::Triangular triangular;
    triangular.AddDepthSubscirbe("btcusdt", false);
    triangular.AddDepthSubscirbe("ethusdt", false);
    triangular.AddDepthSubscirbe("ethbtc", false);

    triangular.AddDepthSubscirbe("xrpbtc", false);
    triangular.AddDepthSubscirbe("xrpusdt", false);
    triangular.AddDepthSubscirbe("xrpeth", true);

    triangular.AddDepthSubscirbe("ltcbtc", false);
    triangular.AddDepthSubscirbe("ltcusdt", true);
    triangular.AddDepthSubscirbe("ltceth", false);

    triangular.AddDepthSubscirbe("maticbtc", false);
    triangular.AddDepthSubscirbe("maticusdt", true);
    triangular.AddDepthSubscirbe("maticeth", false);

    triangular.run();
    return 0;
}
