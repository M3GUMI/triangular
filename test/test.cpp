#include <iostream>
#include <sys/timeb.h>
#include "lib/api/http/binance_api_wrapper.h"
#include "lib/api/ws/binance_depth_wrapper.h"
#include "lib/api/ws/binance_order_wrapper.h"
#include "lib/executor/executor.h"

int main()
{
    websocketpp::lib::asio::io_service ioService;
    HttpWrapper::BinanceApiWrapper apiWrapper(ioService);
    WebsocketWrapper::BinanceDepthWrapper depthWrapper(ioService, apiWrapper, "stream.binance.com", "9443");
    WebsocketWrapper::BinanceOrderWrapper orderWrapper(ioService, apiWrapper, "stream.binance.com", "9443");
    // apiWrapper.InitBinanceSymbol();

    Pathfinder::Pathfinder pathfinder(depthWrapper);
    CapitalPool::CapitalPool capitalPool(pathfinder, apiWrapper);
    Executor::Executor executor(pathfinder, capitalPool, apiWrapper);

    pathfinder.MockRun();
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
