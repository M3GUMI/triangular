#include "binanceorderwebsocketclient.h"




binanceorderwebsocketclient::binanceorderwebsocketclient(std::string conectionName, websocketpp::lib::asio::io_service *ioService, string msg)
{
}
binanceorderwebsocketclient::~binanceorderwebsocketclient()
{
}
 void binanceorderwebsocketclient::WSMsgHandler(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg)
    {
             // cout << __func__ << " " << __LINE__ << "WSMsgHandler : " << msg->get_payload().c_str() << endl;
        rapidjson::Document wsMsgInfoJson;
        wsMsgInfoJson.Parse(msg->get_payload().c_str());

        if (not wsMsgInfoJson.HasMember("e"))
        {
            cout << __func__ << " " << __LINE__ << "unknown msg " << msg->get_payload().c_str() << endl;
            return;
        }

        std::string e = wsMsgInfoJson.FindMember("e")->value.GetString();
        
        if (e == "executionReport")
        {
            return ExecutionReportHandler(wsMsgInfoJson);
        }

        cout << __func__ << " " << __LINE__ << "unkown msg type " << e << endl;
        cout << __func__ << " " << __LINE__ << "[" << msg->get_payload().c_str() << "]" << endl;

    }

    void binanceorderwebsocketclient::ExecutionReportHandler(const rapidjson::Document &msg)
    {
                std::string symbol = msg.FindMember("s")->value.GetString();
        std::string from, to;
        std::string side = msg.FindMember("S")->value.GetString();
        std::string ori = msg.FindMember("q")->value.GetString();
        std::string exced = msg.FindMember("z")->value.GetString();
        double doubleExced = 0, doubleOri = 0;
        String2Double(exced, doubleExced);
        String2Double(ori, doubleOri);
        std::string status = msg.FindMember("X")->value.GetString();

        from = mapSymbol2Base[symbol].first;
        to = mapSymbol2Base[symbol].second;

        cout << __func__ << " " << __LINE__ << " " << symbol << " ExecutionReportHandler " << side << " " << ori << " " << exced << endl;

        if (side == "BUY")
        {
            swap(from, to);
        }

    }