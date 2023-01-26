#pragma once
#include <string.h>
#include <rapidjson/document.h>
#include "websocket_wrapper.h"

using namespace std;
namespace websocketclient
{
    struct OrderData {
        string OrderId;
        string OrderStatus;
        uint64_t UpdateTime;
    };

    class BinanceOrderWrapper : public WebsocketWrapper
    {
    private:
        string listenKey = "";
		function<void(OrderData& data)> subscriber = NULL;
        HttpApi::BinanceApiWrapper& apiWrapper;

        void createListenKeyHandler(int errCode, string listenKey);
        void keepListenKeyHandler();
        void msgHandler(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg);
        void executionReportHandler(const rapidjson::Document &msg);

    public:
        BinanceOrderWrapper(websocketpp::lib::asio::io_service& ioService, HttpApi::BinanceApiWrapper& binanceApiWrapper);
        ~BinanceOrderWrapper();

        void Connect();
        void SubscribeOrder(function<void(OrderData& req)> handler);
    };
}