#include "websocket_wrapper.h"

namespace websocketclient
{

    WebsocketWrapper::WebsocketWrapper(const std::string conectionName, websocketpp::lib::asio::io_service *ioService, std::string msg) : conectionName(conectionName), ioService(ioService), on_open_send_msg(msg)
    {
    }

    WebsocketWrapper::WebsocketWrapper()
    {
    }

    WebsocketWrapper::~WebsocketWrapper()
    {
    }

    void WebsocketWrapper::Connect(string uri, function<void(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg)> msgHandler)
    {
        try
        {
            client.clear_access_channels(websocketpp::log::alevel::all);
            client.set_error_channels(websocketpp::log::elevel::all);

            if (ioService != NULL)
            {
                client.init_asio(ioService);
            }
            else
            {
                client.init_asio();
            }

            client.set_message_handler(msgHandler);
            client.set_tls_init_handler(websocketpp::lib::bind(&WebsocketWrapper::on_tls_init, this, websocketpp::lib::placeholders::_1));
            client.set_open_handler(websocketpp::lib::bind(&WebsocketWrapper::on_open, this, websocketpp::lib::placeholders::_1));
            websocketpp::lib::error_code ec;

            std::string uri = "wss://" + hostname + ":" + hostport + "/ws" + uri;
            cout << "wss get connection : " << uri << endl;
            auto con = client.get_connection(uri, ec);
            if (ec)
            {
                std::cout << "could not create connection because: " << ec.message() << std::endl;
                return;
            }

            client.connect(con);
        }
        catch (websocketpp::exception const &e)
        {
            std::cout << e.what() << std::endl;
        }

        std::cout << "connection end" << std::endl;
    }

    websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> WebsocketWrapper::on_tls_init(websocketpp::connection_hdl)
    {
        auto ssl_ctx = websocketpp::lib::make_shared<websocketpp::lib::asio::ssl::context>(websocketpp::lib::asio::ssl::context::sslv23_client);
        ssl_ctx->set_verify_mode(websocketpp::lib::asio::ssl::verify_none);
        return ssl_ctx;
    }

    void WebsocketWrapper::on_open(websocketpp::connection_hdl hdl)
    {
        // todo ping timer
        // ping_timer = std::make_shared<websocketpp::lib::asio::steady_timer>(*_ios, websocketpp::lib::asio::milliseconds(10000));
        // ping_timer->async_wait(websocketpp::lib::bind(&BaseExchange::on_ping_timer, this, websocketpp::lib::placeholders::_1));
        hdl = hdl;
        send(on_open_send_msg);
    }

    void WebsocketWrapper::send(const std::string &data)
    {
        if (data.empty())
        {
            return;
        }

        websocketpp::lib::error_code ec;
        client.send(hdl, data, websocketpp::frame::opcode::text, ec);
        std::cout << "send info " << data << std::endl;
    }
}