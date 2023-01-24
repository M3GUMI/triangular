#pragma once
#include <string.h>
#include "websocket_wrapper.h"

using namespace std;
namespace websocketclient
{
    struct OrderData {
        string OrderId;
        string OrderStatus;
        uint64_t UpdateTime;
    };

    class BinanceOrderWraper : public WebsocketWrapper
    {
    private:
        string listenKey = "";
		function<void(OrderData& data)> subscriber = NULL;
        std::map<string, std::pair<std::string, std::string>> mapSymbol2Base; // 交易对到货币名

        void createListenKeyHandler(int errCode, string listenKey);
        void keepListenKeyHandler();
        void msgHandler(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg);
        void executionReportHandler(const rapidjson::Document &msg);

    public:
        BinanceOrderWraper();
        ~BinanceOrderWraper();

        // 创建连接
        void Connect();
        // 订阅订单数据
        void SubscribeOrder(function<void(OrderData& req)> handler);
    };
}