#include "binancedepthwebsocketclient.h"
#include "binanceorderwebsocketclient.h"



void binancedepthwebsocketclient::CreateAndSubListenKey()
{
    std::string uri = "https://api.binance.com/api/v3/userDataStream";
    restapi_client->MakeRequest({}, "POST", uri, "", std::bind(&binancedepthwebsocketclient::CreateListkeyHandler, this, ::_1, ::_2), true);
}

void binancedepthwebsocketclient::CreateListkeyHandler(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec)
{
    uint64_t next_keep_time_ms = 1000 * 60 * 20;
        if (res == nullptr || res->payload().empty())
        {
            cout << __func__ << " " << __LINE__ << " CreateListkey error " << endl;
            next_keep_time_ms = 1000 * 60;
            // return;
        }

        auto &msg = res->payload();
        if (res->http_status() != 200)
        {
            cout << __func__ << " " << __LINE__ << "***** CreateListkey http status error *****" << res->http_status() << " msg: " << msg << endl;
            next_keep_time_ms = 1000 * 60;
            // return;
        }

        rapidjson::Document listenKeyInfo;
        listenKeyInfo.Parse(msg.c_str());

        if (listenKeyInfo.HasMember("listenKey"))
        {
            listenKey = listenKeyInfo.FindMember("listenKey")->value.GetString();
            cout << __func__ << " " << __LINE__ << "get listen key " << listenKey << endl;
            ws_client = new binancedepthwebsocketclient("", &ioService, "");
            ws_client->Connect(bind(&binanceorderwebsocketclient::WSMsgHandler, this, ::_1, ::_2), string("/") + listenKey);
        }

        listenkeyKeepTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService, websocketpp::lib::asio::milliseconds(next_keep_time_ms));
        listenkeyKeepTimer->async_wait(std::bind(&binancedepthwebsocketclient::listenkeyKeepHandler, this));

}

 void binancedepthwebsocketclient::listenkeyKeepHandler()
 {
     cout << __func__ << " " << __LINE__ << "Begin keep listenkey" << endl;
        map<std::string, std::string> args;
        args["listenKey"] = listenKey;
        std::string uri = "https://api.binance.com/api/v3/userDataStream";
        restapi_client->MakeRequest(args, "PUT", uri, "", std::bind(&CreateListkeyHandler, this, ::_1, ::_2), false);
 
 }