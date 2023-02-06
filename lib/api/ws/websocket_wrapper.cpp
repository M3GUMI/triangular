#include "websocket_wrapper.h"
#include "utils/utils.h"

namespace WebsocketWrapper
{

    WebsocketWrapper::WebsocketWrapper(string hostname, string hostport, websocketpp::lib::asio::io_service& ioService) : hostname(hostname), hostport(hostport), ioService(ioService)
    {
    }

    WebsocketWrapper::~WebsocketWrapper()
    {
    }

    int WebsocketWrapper::Connect(string uri, string msg, function<void(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg)> msgHandler)
    {
        try
        {
            this->sendMsg = msg;
            client.clear_access_channels(websocketpp::log::alevel::all);
            client.set_error_channels(websocketpp::log::elevel::all);

            client.init_asio(&ioService);
            client.set_message_handler(msgHandler);
            client.set_tls_init_handler(websocketpp::lib::bind(&WebsocketWrapper::on_tls_init, this, websocketpp::lib::placeholders::_1));
            client.set_open_handler(websocketpp::lib::bind(&WebsocketWrapper::on_open, this, websocketpp::lib::placeholders::_1));
            websocketpp::lib::error_code ec;

            std::string clientUri = "wss://" + hostname + ":" + hostport + "/ws" + uri;
            auto con = client.get_connection(clientUri, ec);
            if (ec)
            {
                return ErrorCreateWebsocketFail;
            }

            client.connect(con);
        }
        catch (websocketpp::exception const &e)
        {
            // std::cout << e.what() << std::endl;
            return ErrorCreateWebsocketFail;
        }
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
        this->hdl = hdl;
        send(sendMsg);
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