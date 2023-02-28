#pragma once
#include <functional>
#include "utils/utils.h"
#include "lib/arbitrage/triangular/ioc_triangular.h"
#include "lib/pathfinder/pathfinder.h"

using namespace std;
namespace Executor
{
  class Executor
  {
  private:
    Pathfinder::Pathfinder &pathfinder;
    CapitalPool::CapitalPool &capitalPool;
    HttpWrapper::BinanceApiWrapper &apiWrapper;

    bool lock = false; // 同时只执行一个套利任务
    void arbitragePathHandler(Pathfinder::ArbitrageChance &chance);
    void arbitrageFinishHandler();

  public:
    Executor(Pathfinder::Pathfinder &pathfinder, CapitalPool::CapitalPool &capitalPool, HttpWrapper::BinanceApiWrapper &apiWrapper);
    ~Executor();
  };
}