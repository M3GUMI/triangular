#include "lib/api/http/binance_api_wrapper.h"
#include "lib/order/order.h"
#include "utils/utils.h"
#include "binance_order_wrapper.h"

namespace WebsocketWrapper
{
    BinanceOrderWrapper::BinanceOrderWrapper(websocketpp::lib::asio::io_service& ioService, HttpWrapper::BinanceApiWrapper& binanceApiWrapper, string hostname, string hostport): WebsocketWrapper(hostname, hostport, ioService), apiWrapper(binanceApiWrapper)
    {
        apiWrapper.CreateListenKey("", bind(&BinanceOrderWrapper::createListenKeyHandler, this, placeholders::_1, placeholders::_2));
    }

    BinanceOrderWrapper::~BinanceOrderWrapper()
    {
    }

    void BinanceOrderWrapper::SubscribeOrder(function<void(OrderData &req, int err)> handler)
    {
        this->orderSubscriber.push_back(handler);
    }

    void BinanceOrderWrapper::createListenKeyHandler(string listenKey, int err)
    {
        if (err == 0) {
            // string msg = R"({"method":"SUBSCRIBE","params":[],"id":)" + to_string(time(NULL) % 1000) + R"(})";
            string msg = R"({"method":"SUBSCRIBE","params":[")" + toLower("USD") + R"(@depth5@100ms"],"id":)" + to_string(time(NULL) % 1000) + R"(})";
            this->listenKey = listenKey;
            WebsocketWrapper::Connect("/"+listenKey, msg, bind(&BinanceOrderWrapper::msgHandler, this, placeholders::_1, placeholders::_2));
            keepListenKeyHandler(false);
        } else {
            // todo 不能一直发请求，ip会被ban
            // apiWrapper.CreateListenKey("", bind(&BinanceOrderWrapper::createListenKeyHandler, this, placeholders::_1, placeholders::_2));
        }
    }

    void BinanceOrderWrapper::keepListenKeyHandler(bool call)
    {
        /*uint64_t next_keep_time_ms = 1000 * 60 * 20;
        auto listenkeyKeepTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(this->ioService,
                                                                                         websocketpp::lib::asio::milliseconds(
                                                                                                 next_keep_time_ms));
        listenkeyKeepTimer->async_wait(bind(&BinanceOrderWrapper::keepListenKeyHandler, this, true));

        auto func = [](string listenKey, int err) -> void{};
        apiWrapper.CreateListenKey(this->listenKey, func);*/
    }

    void BinanceOrderWrapper::msgHandler(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg)
    {
        rapidjson::Document msgInfoJson;
        msgInfoJson.Parse(msg->get_payload().c_str());

        if (not msgInfoJson.HasMember("e"))
        {
            return;
        }

        std::string e = msgInfoJson.FindMember("e")->value.GetString();

        if (e != "executionReport")
        {
            return;
        }

        return executionReportHandler(msgInfoJson);
    }

    void BinanceOrderWrapper::executionReportHandler(const rapidjson::Document &msg)
    {
        string clientOrderID = msg.FindMember("c")->value.GetString(); // orderId
        string symbol = msg.FindMember("s")->value.GetString();        // 交易对
        string side = msg.FindMember("S")->value.GetString();        // 交易对
        uint64_t updateTime = msg.FindMember("T")->value.GetUint64();
        string status = msg.FindMember("X")->value.GetString(); // 订单状态
        auto symbolData = this->apiWrapper.GetSymbolData(symbol);

        double price = String2Double(msg.FindMember("p")->value.GetString());
        double quantity = String2Double(msg.FindMember("q")->value.GetString());

        double executeQuantity = String2Double(msg.FindMember("z")->value.GetString());
        double cummulativeQuoteQty = String2Double(msg.FindMember("Z")->value.GetString());

        OrderData data;
        data.OrderId = apiWrapper.GetOrderId(clientOrderID);
        data.BaseToken = symbolData.BaseToken;
        data.QuoteToken = symbolData.QuoteToken;
        data.Side = stringToSide(side);

        data.OrderStatus = stringToOrderStatus(status);
        data.BaseToken = symbolData.BaseToken;
        data.QuoteToken = symbolData.QuoteToken;

        data.Price = price; // 使用内存中数据
        data.Quantity = quantity;

        data.ExecutePrice = price;
        data.ExecuteQuantity = executeQuantity;
        data.CummulativeQuoteQuantity = cummulativeQuoteQty;

        data.UpdateTime = updateTime;

        spdlog::debug(
                "func: executionReportHandler, symbol: {}, side: {}, status: {], price: {}, quantity: {}, executeQuantity: {}, newQuantity: {}",
                symbol, side, data.OrderStatus, data.Price, data.Quantity, data.GetExecuteQuantity(), data.GetNewQuantity()
        );

        // todo 需要内存管理
        for (auto func : this->orderSubscriber)
        {
            func(data, 0);
        }
    }
}