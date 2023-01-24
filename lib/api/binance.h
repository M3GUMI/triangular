#pragma once
#include "./http/binance_api_wrapper.h"
#include "./ws/binance_depth_wrapper.h"

namespace binance
{
    class binance
    {
    public:
        binance(std::string accessKey, std::string secretKey) {}
        ~binance() {}

        // 构建wrapper的参数
        websocketpp::lib::asio::io_service ioService;
        std::string accessKey, secretKey;
        void init();
        void createOrder(HttpApi::CreateOrderReq &req);
        void cancelOrder(std::string orderId);
        void subscribeDepth(std::string fromToken, std::string toToken);
    };
}