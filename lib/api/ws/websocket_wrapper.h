#pragma once
#include <functional>
#include "websocketpp/client.hpp"
#include "websocketpp/config/asio_client.hpp"
#include "define/define.h"

using namespace std;

namespace WebsocketWrapper
{

    class WebsocketWrapper
    {
    public:
        int Connect(string uri, string msg, function<void(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg)> msgHandler);
        define::SocketStatus Status;

    private:
        // 连接配置
        string hostname;
        string hostport;
        string sendMsg;
        string uri;

        std::shared_ptr<websocketpp::lib::asio::steady_timer> pingTimer;

        websocketpp::connection_hdl hdl;
        websocketpp::client<websocketpp::config::asio_tls_client> client;
        void on_disconnect(websocketpp::connection_hdl hdl);
        // 事件触发函数
        websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> on_tls_init(websocketpp::connection_hdl); // ssl安全 http转https
        void on_open(websocketpp::connection_hdl hdl);
        void on_fail(websocketpp::connection_hdl hdl);
        void on_ping_timer();
        void send(const string &data);
    protected:
        // 连接
        websocketpp::lib::asio::io_service &ioService;

        WebsocketWrapper(string hostname, string hostport, websocketpp::lib::asio::io_service &ioService);
        ~WebsocketWrapper();
    };
}
