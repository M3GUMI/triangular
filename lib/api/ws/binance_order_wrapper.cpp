#include "lib/api/http/binance_api_wrapper.h"
#include "utils/utils.h"
#include "binance_order_wrapper.h"

namespace WebsocketWrapper
{
    BinanceOrderWrapper::BinanceOrderWrapper(websocketpp::lib::asio::io_service& ioService, HttpWrapper::BinanceApiWrapper& binanceApiWrapper, string hostname, string hostport): WebsocketWrapper(hostname, hostport, ioService), apiWrapper(binanceApiWrapper)
    {
    }

    BinanceOrderWrapper::~BinanceOrderWrapper()
    {
    }

    void BinanceOrderWrapper::SubscribeOrder(function<void(OrderData &req)> handler)
    {
        this->orderSubscriber.push_back(handler);
    }

    void BinanceOrderWrapper::Connect()
    {
        auto binance = HttpWrapper::BinanceApiWrapper(this->ioService);
        binance.CreateListenKey("", bind(&BinanceOrderWrapper::createListenKeyHandler, this, placeholders::_1, placeholders::_2));
    }

    void BinanceOrderWrapper::createListenKeyHandler(string listenKey, int err)
    {
        string msg = R"({"method":"SUBSCRIBE","params":[],"id":)" + to_string(time(NULL) % 1000) + R"(})";
        WebsocketWrapper::Connect(listenKey, msg, bind(&BinanceOrderWrapper::msgHandler, this, placeholders::_1, placeholders::_2));

        uint64_t next_keep_time_ms = err > 0 ? 1000 * 60:1000 * 60 * 20;
        auto listenkeyKeepTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(this->ioService, websocketpp::lib::asio::milliseconds(next_keep_time_ms));
        listenkeyKeepTimer->async_wait(bind(&BinanceOrderWrapper::keepListenKeyHandler, this));
    }

    void BinanceOrderWrapper::keepListenKeyHandler()
    {
        auto binance = HttpWrapper::BinanceApiWrapper(this->ioService);
        binance.CreateListenKey(this->listenKey, bind(&BinanceOrderWrapper::createListenKeyHandler, this, placeholders::_1, placeholders::_2));
    }

    void BinanceOrderWrapper::msgHandler(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg)
    {
        rapidjson::Document msgInfoJson;
        msgInfoJson.Parse(msg->get_payload().c_str());

        if (not msgInfoJson.HasMember("e"))
        {
            spdlog::error("func: {}, err: {}", "msgHandler", WrapErr(define::ErrorInvalidResp));
            return;
        }

        std::string e = msgInfoJson.FindMember("e")->value.GetString();

        if (e != "executionReport")
        {
            spdlog::error("func: {}, err: {}", "msgHandler", WrapErr(define::ErrorInvalidResp));
            return;
        }

        return executionReportHandler(msgInfoJson);
    }

    void BinanceOrderWrapper::executionReportHandler(const rapidjson::Document &msg)
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
        uint64_t updateTime = GetNowTime();

        auto symbolData = this->apiWrapper.GetSymbolData(symbol);
        from = symbolData.BaseToken;
        to = symbolData.QuoteToken;

        if (side == "BUY")
        {
            swap(from, to);
        }

        OrderData data;
        data.OrderId = apiWrapper.GetOrderId(clientOrderID);
        data.OrderStatus = status;
        data.UpdateTime = updateTime;

        for (auto func : this->orderSubscriber)
        {
            func(data);
        }
    }
}