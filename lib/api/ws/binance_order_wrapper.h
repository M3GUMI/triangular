#pragma once
#include <string.h>
#include <rapidjson/document.h>
#include "websocket_wrapper.h"

using namespace std;
namespace WebsocketWrapper
{
    class BinanceOrderWrapper : public WebsocketWrapper
    {
    private:
        string listenKey = "";
        vector<function<void(OrderData &data)>> orderSubscriber;
        HttpWrapper::BinanceApiWrapper &apiWrapper;

        void createListenKeyHandler(string listenKey, int err);
        void keepListenKeyHandler(bool call);
        void msgHandler(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg);
        void executionReportHandler(const rapidjson::Document &msg);

    public:
        BinanceOrderWrapper(websocketpp::lib::asio::io_service& ioService, HttpWrapper::BinanceApiWrapper& binanceApiWrapper, string hostname, string hostport);
        ~BinanceOrderWrapper();

        void SubscribeOrder(function<void(OrderData& req)> handler);
    };
}