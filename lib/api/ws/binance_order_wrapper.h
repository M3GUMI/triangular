#pragma once
#include <string.h>
#include <rapidjson/document.h>
#include "websocket_wrapper.h"

using namespace std;
namespace WebsocketWrapper
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
        HttpWrapper::BinanceApiWrapper& apiWrapper;

        void createListenKeyHandler(string listenKey, int err);
        void keepListenKeyHandler();
        void msgHandler(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg);
        void executionReportHandler(const rapidjson::Document &msg);

    public:
        BinanceOrderWrapper(websocketpp::lib::asio::io_service& ioService, HttpWrapper::BinanceApiWrapper& binanceApiWrapper);
        ~BinanceOrderWrapper();

        void Connect();
        void SubscribeOrder(function<void(OrderData& req)> handler);
    };
}