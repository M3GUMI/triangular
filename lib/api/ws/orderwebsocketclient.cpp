#include "orderwebsocketclient.h"

using namespace websocketclient;

orderwebsocketclient::orderwebsocketclient(const std::string conectionName, websocketpp::lib::asio::io_service *ioService, std::string msg) : conectionName(conectionName), ioService(ioService), on_open_send_msg(msg)
{
}

orderwebsocketclient::~orderwebsocketclient()
{
}

void orderwebsocketclient::WSMsgHandler(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg)
{
    rapidjson::Document wsMsgInfoJson;
    wsMsgInfoJson.Parse(msg->get_payload().c_str());
    ExecutionReportHandler(wsMsgInfoJson);
    cout << "successfully receive ws msg " << endl;
}

void orderwebsocketclient::ExecutionReportHandler(const rapidjson::Document &msg){
    cout << "successfully get ExecutionReportHandler " << endl;
}
