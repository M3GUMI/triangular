#pragma once
#include <vector>
#include "websocket_wrapper.h"

using namespace std;
namespace websocketclient
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
		function<void(DepthData& data)> subscriber = NULL;
        void msgHandler(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg, std::string exchangeName);

    public:
        BinanceDepthWrapper();
        ~BinanceDepthWrapper();

        // 创建连接
        void Connect(string symbol);
        // 订阅depth数据
        void SubscribeDepth(function<void(DepthData& data)> handler);
    };
}