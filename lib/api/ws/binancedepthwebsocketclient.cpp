#include "binancedepthwebsocketclient.h"
#include "binanceorderwebsocketclient.h"
#include "orderwebsocketclient.h"
#include "../http/binancewebrestapiwrapper.h"




struct orReq
{
    std::string OrderId;
    std::string OrderStatus;
    uint64_t UpdateTime;
};



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
        ws_client->Connect(bind(&binancedepthwebsocketclient::WSMsgHandler, this, ::_1, ::_2), string("/") + listenKey);
    }

    listenkeyKeepTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService, websocketpp::lib::asio::milliseconds(next_keep_time_ms));
    listenkeyKeepTimer->async_wait(std::bind(&binancedepthwebsocketclient::listenkeyKeepHandler, this));
}

//时间不够就调用演唱60mins
void binancedepthwebsocketclient::listenkeyKeepHandler()
{
    cout << __func__ << " " << __LINE__ << "Begin keep listenkey" << endl;
    map<std::string, std::string> args;
    args["listenKey"] = listenKey;
    std::string uri = "https://api.binance.com/api/v3/userDataStream";
    restapi_client->MakeRequest(args, "PUT", uri, "", std::bind(&CreateListkeyHandler, this, ::_1, ::_2), false);
}

//处理获得msg
void binancedepthwebsocketclient::WSMsgHandler(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg)
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

//订单更新信息处理器
void binancedepthwebsocketclient::ExecutionReportHandler(const rapidjson::Document &msg)
{
    std::string symbol = msg.FindMember("s")->value.GetString(); // 交易对
    std::string from, to;
    std::string side = msg.FindMember("S")->value.GetString(); //购买方向
    std::string ori = msg.FindMember("q")->value.GetString();//原始数量
    std::string exced = msg.FindMember("z")->value.GetString();// 成交数量
    std::string clientOrderID = msg.FindMember("C")->value.GetString();//orderId
    double doubleExced = 0, doubleOri = 0;
    String2Double(exced, doubleExced);
    String2Double(ori, doubleOri);
    std::string status = msg.FindMember("X")->value.GetString(); //订单状态
    uint64_t updateTime = getTime();
     from = mapSymbol2Base[symbol].first;
    to = mapSymbol2Base[symbol].second;

    cout << __func__ << " " << __LINE__ << " " << symbol << " ExecutionReportHandler " << side << " " << ori << " " << exced << endl;

    if (side == "BUY")
    {
        swap(from, to);
    }
    orReq orReq;
    orReq.OrderId = clientOrderID;
    orReq.OrderStatus = status;
    orReq.UpdateTime = updateTime;
    //等subscribe接口
    std::cout << "订单id: " << orReq.OrderId << endl;

}


void Subscribedepth(std::string fromToken,std::string toToken)
{
    
}


