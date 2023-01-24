#include <rapidjson/document.h>

#include "lib/api/http/binance_api_wrapper.h"
#include "utils/utils.h"
#include "binance_order_wrapper.h"

namespace websocketclient
{
    BinanceOrderWraper::BinanceOrderWraper()
    {
    }

    BinanceOrderWraper::~BinanceOrderWraper()
    {
    }

    void BinanceOrderWraper::SubscribeOrder(function<void(OrderData &req)> handler)
    {
        this->subscriber = handler;
    }

    void BinanceOrderWraper::Connect()
    {
        // todo ioService 逻辑待补充
        websocketpp::lib::asio::io_service *ioService = NULL;

        auto binance = HttpApi::BinanceApiWrapper(NULL);
        binance.CreateListenKey("", bind(&BinanceOrderWraper::createListenKeyHandler, this, placeholders::_1, placeholders::_2));
    }

    void BinanceOrderWraper::createListenKeyHandler(int errCode, string listenKey)
    {
        // todo ioService 逻辑待补充
        websocketpp::lib::asio::io_service *ioService = NULL;

        // todo 不知道缺了个什么头文件
        /*string msg = R"({"method":"SUBSCRIBE","params":[],"id":)" + to_string(time(NULL) % 1000) + R"(})";
        WebsocketWrapper::Connect(listenKey, bind(&BinanceOrderWraper::msgHandler, this, placeholders::_1, placeholders::_2));

        uint64_t next_keep_time_ms = errCode > 0 ? 1000 * 60:1000 * 60 * 20;
        auto listenkeyKeepTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService, websocketpp::lib::asio::milliseconds(next_keep_time_ms));
        listenkeyKeepTimer->async_wait(bind(&BinanceOrderWraper::keepListenKeyHandler, this));*/
    }

    void BinanceOrderWraper::keepListenKeyHandler()
    {
        // todo ioService 逻辑待补充
        websocketpp::lib::asio::io_service *ioService = NULL;

        auto binance = HttpApi::BinanceApiWrapper(ioService);
        binance.CreateListenKey(this->listenKey, bind(&BinanceOrderWraper::createListenKeyHandler, this, placeholders::_1, placeholders::_2));
    }

    void BinanceOrderWraper::msgHandler(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg)
    {
        rapidjson::Document msgInfoJson;
        msgInfoJson.Parse(msg->get_payload().c_str());

        if (not msgInfoJson.HasMember("e"))
        {
            cout << __func__ << " " << __LINE__ << "unknown msg " << msg->get_payload().c_str() << endl;
            return;
        }

        std::string e = msgInfoJson.FindMember("e")->value.GetString();

        if (e == "executionReport")
        {
            return executionReportHandler(msgInfoJson);
        }

        cout << __func__ << " " << __LINE__ << "unkown msg type " << e << endl;
        cout << __func__ << " " << __LINE__ << "[" << msg->get_payload().c_str() << "]" << endl;
    }

    void BinanceOrderWraper::executionReportHandler(const rapidjson::Document &msg)
    {
        std::string from, to;
        std::string symbol = msg.FindMember("s")->value.GetString();        // 交易对
        std::string side = msg.FindMember("S")->value.GetString();          // 购买方向
        std::string ori = msg.FindMember("q")->value.GetString();           // 原始数量
        std::string exced = msg.FindMember("z")->value.GetString();         // 成交数量
        std::string clientOrderID = msg.FindMember("C")->value.GetString(); // orderId
        double doubleExced = 0, doubleOri = 0;
        String2Double(exced, doubleExced);
        String2Double(ori, doubleOri);
        std::string status = msg.FindMember("X")->value.GetString(); // 订单状态
        uint64_t updateTime = getTime();
        from = mapSymbol2Base[symbol].first;
        to = mapSymbol2Base[symbol].second;

        cout << __func__ << " " << __LINE__ << " " << symbol << " ExecutionReportHandler " << side << " " << ori << " " << exced << endl;

        if (side == "BUY")
        {
            swap(from, to);
        }

        OrderData data;
        data.OrderId = clientOrderID; // todo 加个映射转换
        data.OrderStatus = status;
        data.UpdateTime = updateTime;
        return this->subscriber(data);
    }
}