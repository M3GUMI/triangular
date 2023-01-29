#pragma once
#include "./http/binance_api_wrapper.h"
#include "./ws/binance_depth_wrapper.h"
#include "./ws/binance_order_wrapper.h"

namespace API
{
    extern HttpWrapper::BinanceApiWrapper *api;
    extern WebsocketWrapper::BinanceDepthWrapper *depth;
    extern WebsocketWrapper::BinanceOrderWrapper *order;

    void Init();
    HttpWrapper::BinanceApiWrapper& GetBinanceApiWrapper();
    WebsocketWrapper::BinanceDepthWrapper& GetBinanceDepthWrapper();
    WebsocketWrapper::BinanceOrderWrapper& GetBinanceOrderWrapper();
}