#include <iostream>
#include <rapidjson/document.h>
#include "binance_depth_wrapper.h"
#include "utils/utils.h"

using namespace std;
namespace WebsocketWrapper
{
    BinanceDepthWrapper::BinanceDepthWrapper(websocketpp::lib::asio::io_service& ioService, HttpWrapper::BinanceApiWrapper& binanceApiWrapper, string hostname, string hostport): WebsocketWrapper(hostname, hostport, ioService), apiWrapper(binanceApiWrapper)
    {
    }

    BinanceDepthWrapper::~BinanceDepthWrapper()
    {
    }

    void BinanceDepthWrapper::SubscribeDepth(function<void(DepthData& data)> handler)
    {
        this->depthSubscriber.push_back(handler);
    }

    int BinanceDepthWrapper::Connect(string symbol)
    {
        string msg = R"({"method":"SUBSCRIBE","params":[")" + toLower(symbol) + R"(@bookTicker"],"id":)" + to_string(time(NULL) % 1000) + R"(})";
        return WebsocketWrapper::Connect("", msg, bind(&BinanceDepthWrapper::msgHandler, this, placeholders::_1, placeholders::_2, symbol));
    }

    void BinanceDepthWrapper::msgHandler(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg, string symbol)
    {
        rapidjson::Document depthInfoJson;
        depthInfoJson.Parse(msg->get_payload().c_str());

        if (not depthInfoJson.HasMember("u")) {
            return;
        }

        if (this->lastUpdateId < depthInfoJson["u"].GetUint64())
        {
            this->lastUpdateId = depthInfoJson["u"].GetUint64();
        } else {
            return;
        }

        DepthData data;
        auto symbolData = this->apiWrapper.GetSymbolData(symbol);
        data.BaseToken = symbolData.BaseToken;
        data.QuoteToken = symbolData.QuoteToken;
        data.UpdateTime = GetNowTime();

        DepthItem askData{};
        askData.Price = String2Double(depthInfoJson["a"].GetString());
        askData.Quantity = String2Double(depthInfoJson["A"].GetString());
        data.Asks.push_back(askData);

        DepthItem bidData{};
        bidData.Price = String2Double(depthInfoJson["b"].GetString());
        bidData.Quantity = String2Double(depthInfoJson["B"].GetString());
        data.Bids.push_back(bidData);

        for (const auto& func : this->depthSubscriber)
        {
            func(data);
        }
    }

    /*void BinanceDepthWrapper::msgHandler(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg, string symbol)
    {
        rapidjson::Document depthInfoJson;
        depthInfoJson.Parse(msg->get_payload().c_str());

        if (depthInfoJson.FindMember("bids") == depthInfoJson.MemberEnd())
        {
            return;
        }

        if (this->lastUpdateId < depthInfoJson["lastUpdateId"].GetUint64())
        {
            this->lastUpdateId = depthInfoJson["lastUpdateId"].GetUint64();
        } else {
            return;
        }

        const auto &bids = depthInfoJson["bids"];
        const auto &asks = depthInfoJson["asks"];

        if (bids.Size() < 0 || asks.Size() < 0 || bids[0].Size() < 2 || asks[0].Size() < 0)
        {
            spdlog::error("func: {}, err: {}, data: {}", "msgHandler", "depth data error!", msg->get_payload().c_str());
            return;
        }

        DepthData data;
        auto symbolData = this->apiWrapper.GetSymbolData(symbol);
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

        for (auto func : this->depthSubscriber)
        {
            func(data);
        }
    }*/
}