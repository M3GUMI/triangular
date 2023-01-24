#include <iostream>
#include <rapidjson/document.h>
#include "binance_depth_wrapper.h"
#include "utils/utils.h"

using namespace std;
namespace websocketclient
{
    BinanceDepthWrapper::BinanceDepthWrapper()
    {
    }

    BinanceDepthWrapper::~BinanceDepthWrapper()
    {
    }

    void BinanceDepthWrapper::SubscribeDepth(function<void(DepthData& data)> handler)
    {
        this->subscriber = handler;
    }

    void BinanceDepthWrapper::Connect(string symbol)
    {
        string msg = R"({"method":"SUBSCRIBE","params":[")" + symbol + R"(@depth5@100ms"],"id":)" + to_string(time(NULL) % 1000) + R"(})";
        WebsocketWrapper::Connect("", bind(&BinanceDepthWrapper::msgHandler, this, placeholders::_1, placeholders::_2, symbol));
    }

    void BinanceDepthWrapper::msgHandler(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg, std::string exchangeName)
    {
        rapidjson::Document depthInfoJson;
        depthInfoJson.Parse(msg->get_payload().c_str());

        if (depthInfoJson.FindMember("bids") == depthInfoJson.MemberEnd())
        {
            cout << __func__ << " " << __LINE__ << " no depth data, return" << endl;
            return;
        }

        // todo 加一个丢弃冗余数据
        uint64_t lastUpdateId = depthInfoJson["lastUpdateId"].GetUint64();

        const auto &bids = depthInfoJson["bids"];
        const auto &asks = depthInfoJson["asks"];

        if (bids.Size() < 0 || asks.Size() < 0 || bids[0].Size() < 2 || asks[0].Size() < 0)
        {
            cout << __func__ << " " << __LINE__ << "error ! depth data error!" << msg->get_payload().c_str() << endl;
            return;
        }

        DepthItem bidData, askData;
        // for (unsigned i = 0; i < bids.Size(); i++)
        {
            // const auto &bid = bids[i];
            const auto &bid = bids[0];
            double bidPrice = 0, bidValue = 0;
            std::string strBidPrice = bid[0].GetString();
            std::string strBidValue = bid[1].GetString();
            String2Double(strBidPrice.c_str(), bidPrice);
            String2Double(strBidValue.c_str(), bidValue);

            bidData.Price = bidPrice;
            bidData.Quantity = bidValue;
        }

        {
            const auto &ask = asks[0];
            double askPrice = 0, askValue = 0;
            std::string strAskPrice = ask[0].GetString();
            std::string strAskValue = ask[1].GetString();
            String2Double(strAskPrice.c_str(), askPrice);
            String2Double(strAskValue.c_str(), askValue);

            askData.Price = askPrice;
            askData.Quantity = askValue;
        }

        DepthData data;
        data.FromToken = "ETH"; // todo 做一个token映射
        data.ToToken = "USDT";  // todo 做一个token映射
        data.UpdateTime = getTime();
        data.Bids.push_back(bidData);
        data.Asks.push_back(askData);
        this->subscriber(data);
    }
}