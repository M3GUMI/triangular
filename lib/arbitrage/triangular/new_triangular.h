#pragma once
#include "triangular.h"
#include "conf/strategy.h"
#include "lib/api/ws/binance_order_wrapper.h"
#include <functional>

using namespace std;
namespace Arbitrage
{
    // 两次maker挂单三角套利
    class NewTriangularArbitrage : public TriangularArbitrage
    {
    public:
        NewTriangularArbitrage(websocketpp::lib::asio::io_service& ioService,
                                 Pathfinder::Pathfinder& pathfinder,
                                 CapitalPool::CapitalPool& pool,
                                 HttpWrapper::BinanceApiWrapper& apiWrapper);

        ~NewTriangularArbitrage();

        int Run(const string& baseToken, const string& quoteToken, int amount);

    private:
        websocketpp::lib::asio::io_service& ioService;
        WebsocketWrapper::BinanceOrderWrapper* orderWrapper;

        double executeProfit = 1; // 实际执行利润率

        OrderData* PendingOrder = nullptr; // 提前挂单
        int retryTime = 0;
        int currentPhase = 0;
        Pathfinder::TransactionPathItem lastStep{}; // 最后一步挂单数据

        double close = 0.0006; // 撤单重挂阈值
        double open = 0.003; // 挂单阈值

        std::shared_ptr<websocketpp::lib::asio::steady_timer> reorderTimer;//重挂单计时器
        std::shared_ptr<websocketpp::lib::asio::steady_timer> socketReconnectTimer;//socket重连计时器
        void makerOrderChangeHandler(const string& base, const string& quote, double buyPrice, double sellPrice);//价格变化幅度不够大，撤单重挂单

        void takerHandler(OrderData& data);

        void makerHandler(OrderData& data);

        void TransHandler(OrderData& data) override;

        void mockTrader(const string& origin, string step, double buyPrice, double sellPrice);

        void socketReconnectHandler();
    };


}