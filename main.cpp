#include "lib/api/http/binance_api_wrapper.h"
#include "lib/api/ws/binance_order_wrapper.h"
#include "lib/executor/executor.h"
#include "spdlog/spdlog.h"

using namespace std;
int main() {
    spdlog::set_level(spdlog::level::info);
    websocketpp::lib::asio::io_service ioService;
    HttpWrapper::BinanceApiWrapper apiWrapper(ioService);
    Pathfinder::Pathfinder pathfinder(ioService, apiWrapper);
    CapitalPool::CapitalPool capitalPool(ioService, pathfinder, apiWrapper);
    Executor::Executor executor(ioService, pathfinder, capitalPool, apiWrapper);

    apiWrapper.GetOpenOrder();

    apiWrapper.InitBinanceSymbol();
    ioService.run();

    return 0;
}
