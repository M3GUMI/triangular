#pragma once
#include "triangular.h"
#include "lib/api/ws/binance_order_wrapper.h"

using namespace std;
namespace Arbitrage
{
    // 两次maker挂单三角套利
    class MakerTriangularArbitrage : public TriangularArbitrage
    {
    public:
        MakerTriangularArbitrage(websocketpp::lib::asio::io_service& ioService,
                                 WebsocketWrapper::BinanceOrderWrapper &orderWrapper,
                                 Pathfinder::Pathfinder& pathfinder,
                                 CapitalPool::CapitalPool& pool,
                                 HttpWrapper::BinanceApiWrapper& apiWrapper);

        ~MakerTriangularArbitrage();

        int Run(Pathfinder::ArbitrageChance& chance);

        void makerOrderChangeHandler(Pathfinder::TransactionPathItem& lastpath);//价格变化幅度不够大，撤单重挂单
        std::shared_ptr<websocketpp::lib::asio::steady_timer> reorderTimer;//重挂單計時器
    private:
        websocketpp::lib::asio::io_service& ioService;
        WebsocketWrapper::BinanceOrderWrapper &orderWrapper;

        void TransHandler(OrderData& orderData);

        double threshold;

        int partiallyFilledHandler(OrderData& orderData);
    };
}