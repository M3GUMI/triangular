#include "websocketclient.h"

namespace websocketclient
{

    websocketclient::websocketclient(const std::string conectionName, websocketpp::lib::asio::io_service *ioService, std::string msg) : conectionName(conectionName), ioService(ioService), on_open_send_msg(msg)
    {
    }
    websocketclient::websocketclient()
    {
    }
    websocketclient::~websocketclient()
    {
    }

    // void websocketclient::on_message(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg)
    // {
    //     std::cout << m_conectionName << " get msg : " << msg->get_payload() << std::endl;
    // }

    websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> websocketclient::on_tls_init(websocketpp::connection_hdl)
    {
        auto ssl_ctx = websocketpp::lib::make_shared<websocketpp::lib::asio::ssl::context>(websocketpp::lib::asio::ssl::context::sslv23_client);
        ssl_ctx->set_verify_mode(websocketpp::lib::asio::ssl::verify_none);
        return ssl_ctx;
    }

    // 连接时
    void websocketclient::on_open(websocketpp::connection_hdl hdl)
    {
        // todo ping timer
        // ping_timer = std::make_shared<websocketpp::lib::asio::steady_timer>(*_ios, websocketpp::lib::asio::milliseconds(10000));
        // ping_timer->async_wait(websocketpp::lib::bind(&BaseExchange::on_ping_timer, this, websocketpp::lib::placeholders::_1));
        hdl = hdl;

        OnOpenSendMsg();
    }

    // 连接成功发送的信息，决定订阅对象
    void websocketclient::OnOpenSendMsg()
    {
        // std::string msg = R"({"method":"SUBSCRIBE","params":[")" + m_conectionName + R"(@depth5@100ms"],"id":)" + to_string(time(NULL) % 1000) + R"(})";
        Send(on_open_send_msg);
    }
    // 请求连接函数
    void websocketclient::Connect(std::function<void(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg)> msgHandler, std::string URI)
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

            // client.set_message_handler(bind(&websocketclient::on_message, this, ::_1, ::_2));
            client.set_message_handler(msgHandler);
            client.set_tls_init_handler(bind(&websocketclient::on_tls_init, this, ::_1));
            client.set_open_handler(bind(&websocketclient::on_open, this, ::_1));
            websocketpp::lib::error_code ec;

            std::string uri = "wss://" + hostname + ":" + hostport + "/ws" + URI;
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

    // 信息发送函数
    void websocketclient::Send(const std::string &data)
    {
        if (data.empty())
        {
            return;
        }

        websocketpp::lib::error_code ec;
        client.send(hdl, data, websocketpp::frame::opcode::text, ec);
        std::cout << "send info " << data << std::endl;
    }

    void websocketclient::InitSymbol2Base()
    {
    }

    // 工具函数
    static void String2Double(const string &str, double &d)
    {
        std::istringstream s(str);
        s >> d;
    }

    static uint64_t getTime()
    {

        timeb t;
        ftime(&t); // 获取毫秒
        time_t curr =  t.time * 1000 + t.millitm;
        uint64_t time = ((uint64_t)curr);
        return time;
    }
}