#include <functional>
#include "utils/utils.h"
#include "lib/arbitrage/triangular/ioc_triangular.h"
#include "lib/arbitrage/triangular/new_triangular.h"
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
        if (not conf::IsIOC) {
            checkTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService, websocketpp::lib::asio::milliseconds(5 * 1000));
            checkTimer->async_wait(bind(&Executor::initMaker, this));
        }
    }

    void Executor::initMaker()
    {
        checkTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService,
                                                                            websocketpp::lib::asio::milliseconds(10 * 1000));
        checkTimer->async_wait(bind(&Executor::initMaker, this));

        if (this->lock) {
            return;
        }
        this->lock = true;

        apiWrapper.GetUserAsset(bind(&Executor::print, this, placeholders::_1));

        auto* makerTriangular = new Arbitrage::NewTriangularArbitrage(
                ioService, pathfinder, capitalPool, apiWrapper
        );
        auto err = makerTriangular->Run("USDT", "XRP", 20);
        if (err > 0)
        {
            spdlog::error("func: initMaker, err:{}", WrapErr(err));
            this->lock = false;
            return;
        }
        makerTriangular->SubscribeFinish(bind(&Executor::arbitrageFinishHandler, this));
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
        Arbitrage::IocTriangularArbitrage *iocTriangular = new Arbitrage::IocTriangularArbitrage(
                pathfinder,
                capitalPool,
                apiWrapper
        );

        iocTriangular->SubscribeFinish(bind(&Executor::arbitrageFinishHandler, this));
        auto err = iocTriangular->Run(chance);
        if (err > 0) {
            this->lock = false;
            return;
        }
    }

    void Executor::arbitrageFinishHandler() {
        capitalPool.Refresh();
        if (executeTime <= 50){
            this->lock = false;
            spdlog::info("Executor::Over, executeTime:{}",  executeTime);
            executeTime++;
        } else {
            spdlog::info("Executor::Over");
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