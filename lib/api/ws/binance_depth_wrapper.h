#pragma once
#include <vector>
#include "lib/api/http/binance_api_wrapper.h"
#include "websocket_wrapper.h"

using namespace std;
namespace WebsocketWrapper
{
    struct DepthItem
    {
        double Price;    // 价格
        double Quantity; // 数量
    };

    struct DepthData
    {
        std::string FromToken; // 卖出的token
        std::string ToToken;   // 买入的token
        time_t UpdateTime;     // 更新时间，ms精度
        vector<DepthItem> Bids;
        vector<DepthItem> Asks;
    };

    class BinanceDepthWrapper : public WebsocketWrapper
    {
    private:
        uint64_t lastUpdateId = 0;
        function<void(DepthData &data)> subscriber = NULL;
        HttpWrapper::BinanceApiWrapper& apiWrapper;

        void connect(string token0, string token1);
        void symbolReadyHandler(map<string, HttpWrapper::BinanceSymbolData> &data);
        void msgHandler(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg, string token0, string token1);

    public:
        BinanceDepthWrapper(websocketpp::lib::asio::io_service& ioService, HttpWrapper::BinanceApiWrapper& binanceApiWrapper, string hostname, string hostport);
        ~BinanceDepthWrapper();

        void SubscribeDepth(function<void(DepthData &data)> handler);
    };
}