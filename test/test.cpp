#include "lib/triangular/triangular.h"
#include "lib/binance/WebsocketClient.h"
#include "lib/api/binance.h"
#include <iostream>
#include <sys/timeb.h>


int main()
{   

    binance::binance  binance("c52zdrltx6vSMgojFzxJcVQ1v7qiD55G0PgTe31v3fCfEazqgnBu3xNRWOPVOj86", "lDOZfpTNBIG8ICteeNfoOIoOHBONvBsiAP88GJ5rgDMF6bGGPETkM1Ri14mrbkfJ");
    binance.init();
    binance.subscribeDepth("btc", "usdt");
    // std::string a = "234";
    // double b = 1.022;
    // websocketclient::WebsocketClient client;
    // Triangular::Triangular triangular;
    // triangular.AddDepthSubscirbe("btcusdt", false);
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