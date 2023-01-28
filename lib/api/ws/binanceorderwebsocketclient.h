#include "orderwebsocketclient.h"

class binanceorderwebsocketclient : public orderwebsocketclient
{
public:
    binanceorderwebsocketclient(std::string conectionName, websocketpp::lib::asio::io_service *ioService, string msg);
    ~binanceorderwebsocketclient();

    void WSMsgHandler(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg);
   

    void ExecutionReportHandler(const rapidjson::Document &msg);
    
};