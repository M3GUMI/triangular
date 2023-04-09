#pragma once
#include <functional>
#include "utils/utils.h"
#include "lib/arbitrage/triangular/ioc_triangular.h"
#include "lib/pathfinder/pathfinder.h"
#include "lib/pathfinder/graph.h"

using namespace std;
namespace Executor
{
    class Executor
    {
    private:
        websocketpp::lib::asio::io_service& ioService;
        Pathfinder::Pathfinder& pathfinder;
        CapitalPool::CapitalPool& capitalPool;
        HttpWrapper::BinanceApiWrapper& apiWrapper;

        std::shared_ptr<websocketpp::lib::asio::steady_timer> rerunTimer;//重挂单计时器
        std::shared_ptr<websocketpp::lib::asio::steady_timer> checkTimer; // 计时器
        int executeTime = 1;
        bool lock = false; // 同时只执行一个套利任务
        void arbitragePathHandler(Pathfinder::ArbitrageChance& chance);

        void arbitrageFinishHandler();

        void print(double btc);

        void initMaker();

    public:
        Executor(websocketpp::lib::asio::io_service& ioService, Pathfinder::Pathfinder& pathfinder,
                 CapitalPool::CapitalPool& capitalPool, HttpWrapper::BinanceApiWrapper& apiWrapper);

        ~Executor();

        void Init();
    };
}