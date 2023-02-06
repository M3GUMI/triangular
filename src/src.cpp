#include <map>
#include <iostream>
#include <sys/timeb.h>
#include "lib/api/http/binance_api_wrapper.h"
#include "lib/api/ws/binance_depth_wrapper.h"
#include "lib/api/ws/binance_order_wrapper.h"
#include "lib/executor/executor.h"

using namespace std;
int main()
{
    websocketpp::lib::asio::io_service ioService;
    HttpWrapper::BinanceApiWrapper apiWrapper(ioService);
    WebsocketWrapper::BinanceOrderWrapper orderWrapper(ioService, apiWrapper, "stream.binance.com", "9443");

    Pathfinder::Pathfinder pathfinder(ioService, apiWrapper);

    apiWrapper.InitBinanceSymbol();
    // CapitalPool::CapitalPool capitalPool(pathfinder, apiWrapper);
    // Executor::Executor executor(pathfinder, capitalPool, apiWrapper);

    // pathfinder.MockRun();
    ioService.run();

    return 0;
}
