#pragma once
#define ASIO_STANDALONE
#include <bits/stdc++.h>
#include <functional>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/reader.h>
#include <rapidjson/stringbuffer.h>
#include "websocketpp/client.hpp"
#include "boost/asio/ssl/context_base.hpp"
#include "websocketpp/config/asio_client.hpp"
#include <iostream>
#include <sys/timeb.h>

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using namespace std;

namespace websocketclient
{

    class websocketclient
    {

    public:
        websocketclient(std::string conectionName, websocketpp::lib::asio::io_service *ioService, string msg);
        websocketclient::websocketclient();
        ~websocketclient();

        void Connect(std::function<void(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg)> msgHandler, std::string URI);

        // 设定器
        void set_hostname(std::string &hostname);
        void set_hostport(std::string &hostport);
        void set_open_send_msg(std::string &on_open_send_msg);

        // 参数字段：
        // 创建client的时候传入的send_msg将会在open的时候发送，决定订阅的对象
        std::string on_open_send_msg;
        std::string conectionName;
        const std::string hostname = "";
        const std::string hostport = "";

        std::map<std::string, std::pair<std::string, std::string>> mapSymbol2Base; // 交易对到货币名

        // websocketpp
        websocketpp::lib::asio::io_service *ioService;
        websocketpp::client<websocketpp::config::asio_tls_client> client;
        websocketpp::connection_hdl hdl;

        // ssl安全 http转https
        websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> on_tls_init(websocketpp::connection_hdl);

        // 连接状态触发函数：
        void OnOpenSendMsg();
        void on_open(websocketpp::connection_hdl hdl);
        // void on_message(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg);
        void Send(const std::string &data);

        // client中使用的工具函数
        static void String2Double(const string &str, double &d);
        static void InitSymbol2Base();

        static uint64_t getTime() {};
    };

}
