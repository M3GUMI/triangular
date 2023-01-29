#include "api.h"

namespace API
{
    void InitApi()
    {
        websocketpp::lib::asio::io_service ioService;

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
            InitApi();
        }

        return *api;
    }

    WebsocketWrapper::BinanceDepthWrapper& GetBinanceDepthWrapper()
    {
        if (depth == NULL)
        {
            InitApi();
        }

        return *depth;
    }

    WebsocketWrapper::BinanceOrderWrapper &GetBinanceOrderWrapper()
    {
        if (order == NULL)
        {
            InitApi();
        }

        return *order;
    }
}