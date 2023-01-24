#pragma once
#define ASIO_STANDALONE
#include <functional>
#include "websocketpp/client.hpp"
#include "websocketpp/config/asio_client.hpp"

using namespace std;

namespace websocketclient
{

    class WebsocketWrapper
    {
    private:
        // 连接
        websocketpp::lib::asio::io_service *ioService;
        websocketpp::client<websocketpp::config::asio_tls_client> client;
        websocketpp::connection_hdl hdl;

        // 连接配置
        string conectionName;
        string on_open_send_msg;
        const string hostname = "";
        const string hostport = "";

        // 事件触发函数
        websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> on_tls_init(websocketpp::connection_hdl); // ssl安全 http转https
        void on_open(websocketpp::connection_hdl hdl);
        void send(const string &data);

    public:
        WebsocketWrapper(string conectionName, websocketpp::lib::asio::io_service *ioService, string msg);
        WebsocketWrapper();
        ~WebsocketWrapper();

        void Connect(string uri, function<void(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg)> msgHandler);
    };

}
