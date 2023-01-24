#include "./http/binance_api_wrapper.h"
#include "./ws/binance_depth_wrapper.h"
#include "./depthwebsocketclient.h"
#include "binance.h"

namespace binance
{

    binance::binance(std::string accessKey, std::string secretKey)
    {
    }

    binance::~binance()
    {
    }

    void binance::init()
    {
        // todo 编译测试
        /*auto wrapper = HttpApi::BinanceApiWrapper(ioService, accessKey, secretKey);
        wrapper.binanceInitSymbol();
        binancedepthwebsocketclient client = binancedepthwebsocketclient();
        client.CreateAndSubListenKey();*/
    }

    void binance::createOrder(HttpApi::CreateOrderReq& req)
    {
        auto wrapper = HttpApi::BinanceApiWrapper(&ioService);
        wrapper.CreateOrder(req);
    }

    void binance::cancelOrder(std::string orderId)
    {
        auto wrapper = HttpApi::BinanceApiWrapper(&ioService);
        wrapper.CancelOrder(orderId);
    }

    void binance::subscribeDepth(std::string fromToken, std::string toToken)
    {
        // todo 编译测试
        /*auto wrapper = HttpApi::BinanceApiWrapper(ioService, accessKey, secretKey);
        pair pair = wrapper.getSymbolSize(fromToken, toToken);
        std::string connectName = pair.first;
        auto client = binancedepthwebsocketclient(connectName, ioService, "");
        client.AddDepthSubscirbe(connectName);*/
    }
}