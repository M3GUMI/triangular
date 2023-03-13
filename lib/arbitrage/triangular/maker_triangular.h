#pragma once
#include "triangular.h"
#include "conf/strategy.h"
#include "lib/api/ws/binance_order_wrapper.h"

using namespace std;
namespace Arbitrage
{
    // 两次maker挂单三角套利
    class MakerTriangularArbitrage : public TriangularArbitrage
    {
    public:
        MakerTriangularArbitrage(websocketpp::lib::asio::io_service& ioService,
                                 WebsocketWrapper::BinanceOrderWrapper& orderWrapper,
                                 Pathfinder::Pathfinder& pathfinder,
                                 CapitalPool::CapitalPool& pool,
                                 HttpWrapper::BinanceApiWrapper& apiWrapper);

        ~MakerTriangularArbitrage();

        int Run(string baseToken, string quoteToken);

    private:
        websocketpp::lib::asio::io_service& ioService;
        WebsocketWrapper::BinanceOrderWrapper& orderWrapper;

        OrderData* PendingOrder = nullptr; // 提前挂单
        int currentPhase = 0;
        string baseToken;
        string quoteToken;

        double close = 0.01; // 撤单重挂阈值
        double open = 0.005; // 挂单阈值

        std::shared_ptr<websocketpp::lib::asio::steady_timer> reorderTimer;//重挂单计时器
        std::shared_ptr<websocketpp::lib::asio::steady_timer> retryTimer;//市价吃单计时器

        void makerOrderChangeHandler();//价格变化幅度不够大，撤单重挂单
        int partiallyFilledHandler(OrderData& data);
        void takerHandler(OrderData &data);
        void makerHandler(OrderData &data);

        void TransHandler(OrderData& data) override;
    };
}