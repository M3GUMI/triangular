#pragma once
#include <functional>
#include "websocketpp/client.hpp"
#include "websocketpp/config/asio_client.hpp"

using namespace std;

namespace websocketclient
{

    class WebsocketWrapper
    {
    private:
        // 连接配置
        string sendMsg;
        string hostname = "";
        string hostport = "";

        websocketpp::connection_hdl hdl;
        websocketpp::client<websocketpp::config::asio_tls_client> client;

        // 事件触发函数
        websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> on_tls_init(websocketpp::connection_hdl); // ssl安全 http转https
        void on_open(websocketpp::connection_hdl hdl);
        void send(const string &data);

    public:
        // 连接
        websocketpp::lib::asio::io_service& ioService;

        WebsocketWrapper(string hostname, string hostport, websocketpp::lib::asio::io_service& ioService);
        ~WebsocketWrapper();

        void Connect(string uri, string msg, function<void(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg)> msgHandler);
    };

}
