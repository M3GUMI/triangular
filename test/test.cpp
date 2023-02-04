#include <iostream>
#include <sys/timeb.h>
#include "lib/api/api.h"
#include "lib/capital_pool/capital_pool.h"
#include "lib/executor/executor.h"
#include "lib/triangular/triangular.h"

int main()
{
    websocketpp::lib::asio::io_service ioService;
    API::Init(ioService);
    Pathfinder::Init();
    CapitalPool::GetCapitalPool().Refresh();
    // Executor::Executor executor(pathfinder);
    ioService.run();

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
