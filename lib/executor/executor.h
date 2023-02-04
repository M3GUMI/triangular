#pragma once
#include <functional>
#include "utils/utils.h"
#include "lib/arbitrage/triangular/triangular.h"
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

    bool lock; // 同时只执行一个套利任务
    void arbitragePathHandler(Pathfinder::TransactionPath &path);
    void arbitrageFinishHandler();

  public:
    Executor(Pathfinder::Pathfinder &pathfinder, CapitalPool::CapitalPool &capitalPool, HttpWrapper::BinanceApiWrapper &apiWrapper);
    ~Executor();
  };
}