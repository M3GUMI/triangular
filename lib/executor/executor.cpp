#include <functional>
#include "utils/utils.h"
#include "lib/arbitrage/triangular/ioc_triangular.h"
#include "lib/arbitrage/triangular/maker_triangular.h"
#include "executor.h"

using namespace std;
namespace Executor{
    Executor::Executor(
            websocketpp::lib::asio::io_service& ioService,
            Pathfinder::Pathfinder &pathfinder,
            CapitalPool::CapitalPool &capitalPool,
            HttpWrapper::BinanceApiWrapper &apiWrapper
    ) : ioService(ioService), pathfinder(pathfinder), capitalPool(capitalPool), apiWrapper(apiWrapper) {
        pathfinder.SubscribeArbitrage((bind(&Executor::arbitragePathHandler, this, placeholders::_1)));
        pathfinder.SubscribeDepthReady(bind(&Executor::Init, this));
    }

    Executor::~Executor() = default;

    void Executor::Init() {
        apiWrapper.CancelOrderSymbol("XRP", "USDT");
        apiWrapper.CancelOrderSymbol("BUSD", "USDT");
        checkTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService, websocketpp::lib::asio::milliseconds(5 * 1000));
        checkTimer->async_wait(bind(&Executor::initMaker, this));
    }

    void Executor::initMaker()
    {
        checkTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService,
                                                                            websocketpp::lib::asio::milliseconds(
                                                                                    10 * 1000));
        checkTimer->async_wait(bind(&Executor::initMaker, this));

        if (this->lock) {
            return;
        }
        this->lock = true;

        apiWrapper.GetUserAsset(bind(&Executor::print, this, placeholders::_1));

        auto* makerTriangular = new Arbitrage::MakerTriangularArbitrage(
                ioService, pathfinder, capitalPool, apiWrapper
        );
        auto err = makerTriangular->Run("USDT", "XRP", 20);
        if (err > 0)
        {
            this->lock = false;
            return;
        }
        makerTriangular->SubscribeFinish(bind(&Executor::arbitrageFinishHandler, this));
    }

    void Executor::arbitragePathHandler(Pathfinder::ArbitrageChance &chance) {
        return;
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
        capitalPool.Refresh();
        if (executeTime <= 5){
            this->lock = false;
            spdlog::info("func: arbitrageFinishHandler, executeTime:{}",  executeTime);
            executeTime++;
        }

        if (!conf::EnableMock) {
            apiWrapper.GetUserAsset(bind(&Executor::print, this, placeholders::_1));
        }
    }

    void Executor::print(double btc) {
        auto symbolData = apiWrapper.GetSymbolData("USDT", "BTC");
        Pathfinder::GetExchangePriceReq priceReq{};
        priceReq.BaseToken = symbolData.BaseToken;
        priceReq.QuoteToken = symbolData.QuoteToken;
        priceReq.OrderType = define::LIMIT;

        Pathfinder::GetExchangePriceResp priceResp{};
        if (auto err = pathfinder.GetExchangePrice(priceReq, priceResp); err > 0) {
            return;
        }

        spdlog::info("USDT: {}", btc*priceResp.SellPrice);
    }
}