#include "./http/binancewebrestapiwrapper.h"
#include "./ws/binancedepthwebsocketclient.h"

namespace binance
{
    class binance
    {
    public:
        binance(std::string accessKey,std::string secretKey) {}
        ~binance() {}

        // 构建wrapper的参数
        websocketpp::lib::asio::io_service ioService;
        std::string accessKey, secretKey;
        void init();
        void createOrder(binancewebrestapiwrapper::CreateOrderReq sArgs);
        void cancelOrder(binancewebrestapiwrapper::cancelOrderReq cArgs);
        void subscribeDepth(std::string fromToken, std::string toToken);
    };
}