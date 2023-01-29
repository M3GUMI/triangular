#include <iostream>
#include <rapidjson/document.h>
#include "binance_depth_wrapper.h"
#include "utils/utils.h"

using namespace std;
namespace WebsocketWrapper
{
    BinanceDepthWrapper::BinanceDepthWrapper(websocketpp::lib::asio::io_service& ioService, HttpWrapper::BinanceApiWrapper& binanceApiWrapper): WebsocketWrapper("", "", ioService), apiWrapper(binanceApiWrapper)
    {
    }

    BinanceDepthWrapper::~BinanceDepthWrapper()
    {
    }

    void BinanceDepthWrapper::SubscribeDepth(function<void(DepthData& data)> handler)
    {
        this->subscriber = handler;
    }

    void BinanceDepthWrapper::Connect(string token0, string token1)
    {
        auto symbol = this->apiWrapper.GetSymbol(token0, token1);
        string msg = R"({"method":"SUBSCRIBE","params":[")" + symbol + R"(@depth5@100ms"],"id":)" + to_string(time(NULL) % 1000) + R"(})";
        WebsocketWrapper::Connect("", msg, bind(&BinanceDepthWrapper::msgHandler, this, placeholders::_1, placeholders::_2, token0, token1));
    }

    void BinanceDepthWrapper::msgHandler(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg, string token0, string token1)
    {
        rapidjson::Document depthInfoJson;
        depthInfoJson.Parse(msg->get_payload().c_str());

        if (depthInfoJson.FindMember("bids") == depthInfoJson.MemberEnd())
        {
            cout << __func__ << " " << __LINE__ << " no depth data, return" << endl;
            return;
        }

        if (this->lastUpdateId < depthInfoJson["lastUpdateId"].GetUint64()) {
            this->lastUpdateId = depthInfoJson["lastUpdateId"].GetUint64();
            return;
        }

        const auto &bids = depthInfoJson["bids"];
        const auto &asks = depthInfoJson["asks"];

        if (bids.Size() < 0 || asks.Size() < 0 || bids[0].Size() < 2 || asks[0].Size() < 0)
        {
            cout << __func__ << " " << __LINE__ << "error ! depth data error!" << msg->get_payload().c_str() << endl;
            return;
        }

        DepthData data;
        auto symbolData = this->apiWrapper.GetSymbolData(token0, token1);
        data.FromToken = symbolData.BaseToken;
        data.ToToken = symbolData.QuoteToken;
        data.UpdateTime = GetNowTime();

        for (unsigned i = 0; i < bids.Size() && i < 5; i++)
        {
            const auto &bid = bids[i];
            double bidPrice = 0, bidValue = 0;
            std::string strBidPrice = bid[0].GetString();
            std::string strBidValue = bid[1].GetString();
            String2Double(strBidPrice.c_str(), bidPrice);
            String2Double(strBidValue.c_str(), bidValue);

            DepthItem bidData;
            bidData.Price = bidPrice;
            bidData.Quantity = bidValue;
            data.Bids.push_back(bidData);
        }

        for (unsigned i = 0; i < asks.Size() && i < 5; i++)
        {
            const auto &ask = asks[i];
            double askPrice = 0, askValue = 0;
            std::string strAskPrice = ask[0].GetString();
            std::string strAskValue = ask[1].GetString();
            String2Double(strAskPrice.c_str(), askPrice);
            String2Double(strAskValue.c_str(), askValue);

            DepthItem askData;
            askData.Price = askPrice;
            askData.Quantity = askValue;
            data.Asks.push_back(askData);
        }

        this->subscriber(data);
    }
}