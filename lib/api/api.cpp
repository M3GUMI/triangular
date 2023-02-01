#include "api.h"

namespace API
{
    HttpWrapper::BinanceApiWrapper *api;
    WebsocketWrapper::BinanceDepthWrapper *depth;
    WebsocketWrapper::BinanceOrderWrapper *order;

    void Init(websocketpp::lib::asio::io_service& ioService)
    {
        HttpWrapper::BinanceApiWrapper apiWrapper(ioService);
        WebsocketWrapper::BinanceDepthWrapper depthWrapper(ioService, apiWrapper);
        WebsocketWrapper::BinanceOrderWrapper orderWrapper(ioService, apiWrapper);
        apiWrapper.InitBinanceSymbol();

        api = &apiWrapper;
        depth = &depthWrapper;
        order = &orderWrapper;
    }

    HttpWrapper::BinanceApiWrapper &GetBinanceApiWrapper()
    {
        if (api == NULL)
        {
            // 直接重启
        }

        return *api;
    }

    WebsocketWrapper::BinanceDepthWrapper& GetBinanceDepthWrapper()
    {
        if (depth == NULL)
        {
            // 直接重启
        }

        return *depth;
    }

    WebsocketWrapper::BinanceOrderWrapper &GetBinanceOrderWrapper()
    {
        if (order == NULL)
        {
            // 直接重启
        }

        return *order;
    }
}