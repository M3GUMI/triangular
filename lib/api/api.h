#pragma once
#include "./http/binance_api_wrapper.h"
#include "./ws/binance_depth_wrapper.h"
#include "./ws/binance_order_wrapper.h"

namespace API
{
    HttpApi::BinanceApiWrapper *api;
    websocketclient::BinanceDepthWrapper *depth;
    websocketclient::BinanceOrderWrapper *order;

    void InitApi();
    HttpApi::BinanceApiWrapper& GetBinanceApiWrapper();
    websocketclient::BinanceDepthWrapper& GetBinanceDepthWrapper();
    websocketclient::BinanceOrderWrapper& GetBinanceOrderWrapper();
}