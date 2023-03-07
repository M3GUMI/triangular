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
        Pathfinder::Graph& graph;
        CapitalPool::CapitalPool& capitalPool;
        HttpWrapper::BinanceApiWrapper& apiWrapper;

        bool lock = false; // 同时只执行一个套利任务
        void arbitragePathHandler(Pathfinder::ArbitrageChance& chance);

        void arbitrageFinishHandler();

        void print(double btc);

    public:
        Executor(Pathfinder::Graph& graph, CapitalPool::CapitalPool& capitalPool,
                 HttpWrapper::BinanceApiWrapper& apiWrapper);

        ~Executor();
    };
}