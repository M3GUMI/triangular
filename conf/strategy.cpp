#include "strategy.h"

namespace conf
{
    // 交易路径固定
    // a、c为稳定币，b为非稳定币
    // 1. 提前挂单 a->b
    // 2. 市价吃单 b->c
    // 3. 稳定币挂单 c->a
    Strategy MakerTriangular = Strategy{
            .StrategyName =  "MakerTriangular",
            .Steps = vector<Step>{
                    Step{
                        .StableCoin = false,
                        .OrderType = define::LIMIT_MAKER,
                        .TimeInForce = define::GTC
                    },
                    Step{
                        .StableCoin = true,
                        .OrderType = define::MARKET,
                        .TimeInForce = define::IOC
                    },
                    Step{
                        .StableCoin = true,
                        .OrderType = define::LIMIT_MAKER,
                        .TimeInForce = define::GTC
                    }
            }
    };

    Strategy IOCTriangular = Strategy{
            .StrategyName =  "IOCTriangular",
            .Steps = vector<Step>{
                    Step{
                            .StableCoin = false,
                            .OrderType = define::LIMIT,
                            .TimeInForce = define::IOC
                    },
                    Step{
                            .StableCoin = true,
                            .OrderType = define::LIMIT,
                            .TimeInForce = define::IOC
                    },
                    Step{
                            .StableCoin = true,
                            .OrderType = define::LIMIT,
                            .TimeInForce = define::IOC
                    }
            }
    };
}
