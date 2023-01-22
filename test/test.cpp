#include "lib/triangular/triangular.h"
#include "lib/binance/WebsocketClient.h"
#include "binancewebrestapiwrapper.h"
#include <iostream>
#include <sys/timeb.h>


int main()
{   
    // std::string a = "234";
    // double b = 1.022;
    // websocketclient::WebsocketClient client;
    Triangular::Triangular triangular;

    binancewebrestapiwrapper wrapper;
    triangular.AddDepthSubscirbe("btcusdt", false);
    // triangular.AddDepthSubscirbe("ethusdt", false);
    // triangular.AddDepthSubscirbe("ethbtc", false);

    // triangular.AddDepthSubscirbe("xrpbtc", false);
    // triangular.AddDepthSubscirbe("xrpusdt", false);
    // triangular.AddDepthSubscirbe("xrpeth", true);

    // triangular.AddDepthSubscirbe("ltcbtc", false);
    // triangular.AddDepthSubscirbe("ltcusdt", true);
    // triangular.AddDepthSubscirbe("ltceth", false);

    // triangular.AddDepthSubscirbe("maticbtc", false);
    // triangular.AddDepthSubscirbe("maticusdt", true);
    // triangular.AddDepthSubscirbe("maticeth", false);

    // triangular.run();
    // client.Connect("wss://ws-api.binance.com/ws-api/v3");
    // uint64_t num = uint64_t(time(NULL) << 32 | rand );
    // std::cout << num << endl;
    return 0;
}

/*
*
*   todo 余额不足的时候立即开始平衡仓位
*
*/