#pragma once
#define ASIO_STANDALONE
#include <bits/stdc++.h>
#include <functional>

#include "websocketpp/client.hpp"
#include "boost/asio/ssl/context_base.hpp"
#include "websocketpp/config/asio_client.hpp"

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
using namespace std;

namespace BinanceWebsocketWrapper
{

    class BinanceWebsocketWrapper
    {
    public:
        BinanceWebsocketWrapper(std::string conectionName, websocketpp::lib::asio::io_service* ioService, string msg);
        ~BinanceWebsocketWrapper();

        void Connect(std::function<void(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg)> msgHandler, std::string URI);

    private:

        std::string on_open_send_msg;
        std::string m_conectionName;
        websocketpp::lib::asio::io_service* m_ioService;

        websocketpp::client<websocketpp::config::asio_tls_client> client;
        const std::string hostname = "stream.binance.com";
        const std::string hostport = "9443";
        websocketpp::connection_hdl m_hdl;

        void OnOpenSendMsg();
        void on_open(websocketpp::connection_hdl hdl);
        // void on_message(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg);
        websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> on_tls_init(websocketpp::connection_hdl);
        void Send(const std::string &data);
    };

}
