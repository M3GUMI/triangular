#include "api.h"

namespace API
{
    void InitApi()
    {
        websocketpp::lib::asio::io_service ioService;

        HttpApi::BinanceApiWrapper apiWrapper(ioService);
        websocketclient::BinanceDepthWrapper depthWrapper(ioService, apiWrapper);
        websocketclient::BinanceOrderWrapper orderWrapper(ioService, apiWrapper);
        apiWrapper.InitBinanceSymbol();

        api = &apiWrapper;
        depth = &depthWrapper;
        order = &orderWrapper;
    }

    HttpApi::BinanceApiWrapper &GetBinanceApiWrapper()
    {
        if (api == NULL)
        {
            InitApi();
        }

        return *api;
    }

    websocketclient::BinanceDepthWrapper& GetBinanceDepthWrapper()
    {
        if (depth == NULL)
        {
            InitApi();
        }

        return *depth;
    }

    websocketclient::BinanceOrderWrapper &GetBinanceOrderWrapper()
    {
        if (order == NULL)
        {
            InitApi();
        }

        return *order;
    }
}