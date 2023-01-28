#include "websocketclient.h"

using namespace websocketclient;

class orderwebsocketclient : public websocketclient
{
public:
    orderwebsocketclient(std::string conectionName, websocketpp::lib::asio::io_service *ioService, string msg);
    orderwebsocketclient();
    ~orderwebsocketclient();

    // 参数字段：
    // 创建client的时候传入的send_msg将会在open的时候发送，决定订阅的对象
    std::string on_open_send_msg;
    std::string conectionName;
    const std::string hostname = "";
    const std::string hostport = "";

    // websocketpp
    websocketpp::lib::asio::io_service *ioService;
    websocketpp::client<websocketpp::config::asio_tls_client> client;
    websocketpp::connection_hdl hdl;

    // ws channel储存器
    std::map<std::string, websocketclient *> order_ws_client;

    // 订单状态处理器
    void ExecutionReportHandler(const rapidjson::Document &msg);
    void WSMsgHandler(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg);
};