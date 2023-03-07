#include <functional>
#include "utils/utils.h"
#include "lib/arbitrage/triangular/ioc_triangular.h"
#include "executor.h"

using namespace std;
namespace Executor{
    Executor::Executor(
            Pathfinder::Graph &graph,
            CapitalPool::CapitalPool &capitalPool,
            HttpWrapper::BinanceApiWrapper &apiWrapper
    ) : graph(graph), capitalPool(capitalPool), apiWrapper(apiWrapper) {
        graph.SubscribeArbitrage((bind(&Executor::arbitragePathHandler, this, placeholders::_1)));
    }

    Executor::~Executor() {
    }

    void Executor::arbitragePathHandler(Pathfinder::ArbitrageChance &chance) {
        if (this->lock) {
            spdlog::debug("lock");
            return;
        }

        if (!conf::EnableMock) {
            apiWrapper.GetUserAsset(bind(&Executor::print, this, placeholders::_1));
        }

        // todo 此处需要内存管理。需要增加套利任务结束，清除subscribe
        this->lock = true;
        Arbitrage::TriangularArbitrage *iocTriangular = new Arbitrage::IocTriangularArbitrage(
                pathfinder,
                capitalPool,
                apiWrapper
        );
        iocTriangular->SubscribeFinish(bind(&Executor::arbitrageFinishHandler, this));
        if (auto err = iocTriangular->Run(chance); err > 0) {
            this->lock = false;
            return;
        }
    }

    void Executor::arbitrageFinishHandler() {
        // this->lock = false;
        capitalPool.Refresh();
        if (!conf::EnableMock) {
            apiWrapper.GetUserAsset(bind(&Executor::print, this, placeholders::_1));
        }
    }

    void Executor::print(double btc) {
        auto symbolData = apiWrapper.GetSymbolData("BUSD", "BTC");
        Pathfinder::GetExchangePriceReq priceReq{};
        priceReq.BaseToken = symbolData.BaseToken;
        priceReq.QuoteToken = symbolData.QuoteToken;
        priceReq.OrderType = define::LIMIT;

        Pathfinder::GetExchangePriceResp priceResp{};
        if (auto err = pathfinder.GetExchangePrice(priceReq, priceResp); err > 0) {
            return;
        }

        spdlog::info("busd: {}", btc*priceResp.SellPrice);
    }
}