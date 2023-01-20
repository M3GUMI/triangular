#pragma once
#define ASIO_STANDALONE
#include <bits/stdc++.h>
#include <functional>
#include "websocketpp/client.hpp"
#include "boost/asio/ssl/context_base.hpp"
#include "websocketpp/config/asio_client.hpp"
#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>
#include <string>
#include <locale>
#include <codecvt>

typedef websocketpp::client<websocketpp::config::asio_client> client;
typedef std::function<void()> OnOpenFunc;
typedef std::function<void()> OnFailFunc;
typedef std::function<void()> OnCloseFunc;
typedef std::function<void(const std::string &message)> OnMessageFunc;

namespace websocketclient
{
    // 保存一个连接的元数据
    class connection_metadata
    {
    public:
        typedef websocketpp::lib::shared_ptr<connection_metadata> ptr;
        connection_metadata(websocketpp::connection_hdl hdl, std::string url)
            : m_Hdl(hdl), m_Status("Connecting"), m_Url(url), m_Server("N/A")
        {
        }

        // void on_open(client *c, websocketpp::connection_hdl hdl)
        // {
        //     m_Status = "Open";
        //     client::connection_ptr con = c->get_con_from_hdl(hdl);
        //     m_Server = con->get_response_header("Server");
        // }

        // void on_fail(client *c, websocketpp::connection_hdl hdl)
        // {
        //     m_Status = "Failed";
        //     client::connection_ptr con = c->get_con_from_hdl(hdl);
        //     m_Server = con->get_response_header("Server");
        //     m_Error_reason = con->get_ec().message();
        // }

        // void on_close(client *c, websocketpp::connection_hdl hdl)
        // {
        //     m_Status = "Closed";
        //     client::connection_ptr con = c->get_con_from_hdl(hdl);
        //     std::stringstream s;
        //     s << "close code: " << con->get_remote_close_code() << " ("
        //       << websocketpp::close::status::get_string(con->get_remote_close_code())
        //       << "), close reason: " << con->get_remote_close_reason();
        //     m_Error_reason = s.str();
        // }

        // void on_message(websocketpp::connection_hdl, client::message_ptr msg)
        // {
        //     if (msg->get_opcode() == websocketpp::frame::opcode::text)
        //     {
        //         std::string message = msg->get_payload();
        //         std::cout << "收到来自服务器的消息：" << message << std::endl;
        //     }
        //     else
        //     {
        //         std::string message = websocketpp::utility::to_hex(msg->get_payload());
        //     }
        // }

        websocketpp::connection_hdl get_hdl() const
        {
            return m_Hdl;
        }

        std::string get_status() const
        {
            return m_Status;
        }

    private:
        websocketpp::connection_hdl m_Hdl; // websocketpp表示连接的编号
        std::string m_Status;              // 连接状态
        std::string m_Url;                 // 连接的URI
        std::string m_Server;              // 服务器信息
        std::string m_Error_reason;        // 错误原因
    };
    class WebsocketClient
    {
    public:
        WebsocketClient();
        virtual ~WebsocketClient();

    public:
        bool Connect(std::string const &url);
        bool Close(std::string reason = "");
        bool Send(std::string message);

        connection_metadata::ptr GetConnectionMetadataPtr();

        void OnOpen(client *c, websocketpp::connection_hdl hdl);
        void OnFail(client *c, websocketpp::connection_hdl hdl);
        void OnClose(client *c, websocketpp::connection_hdl hdl);
        void OnMessage(websocketpp::connection_hdl, client::message_ptr msg);

        void SetOnOpenFunc(OnOpenFunc func);
        void SetOnFailFunc(OnFailFunc func);
        void SetOnCloseFunc(OnCloseFunc func);
        void SetMessageFunc(OnMessageFunc func);

    private:
        connection_metadata::ptr m_ConnectionMetadataPtr;
        client m_WebsocketClient;
        websocketpp::lib::shared_ptr<websocketpp::lib::thread> m_Thread; // 线程

        OnOpenFunc m_OnOpenFunc;
        OnFailFunc m_OnFailFunc;
        OnCloseFunc m_OnCloseFunc;
        OnMessageFunc m_MessageFunc;
    };
}