#include "depthwebsocketclient.h"


// using namespace websocketclient;

    struct NotifyDepthReq
    {
        std::string FromToken; // 卖出的token
        std::string ToToken;   // 买入的token
        time_t UpdateTime;     // 更新时间，ms精度
        DepthItem Bids[10];
        DepthItem Asks[10];
    };

    struct DepthItem 
    {
        double price; //价格
        double quantity; // 数量
    };

 
        depthwebsocketclient::depthwebsocketclient(const std::string conectionName, websocketpp::lib::asio::io_service *ioService, std::string msg) : conectionName(conectionName), ioService(ioService), on_open_send_msg(msg)
        {
        }
        depthwebsocketclient::depthwebsocketclient()
        {
        }
        depthwebsocketclient::~depthwebsocketclient()
        {
        }

        // depth处理器
        void depthwebsocketclient::DepthMsgHandler(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg, std::string exchangeName)
        {
            cout << __func__ << " " << __LINE__ << msg->get_payload().c_str() << endl;
            rapidjson::Document depthInfoJson;
            depthInfoJson.Parse(msg->get_payload().c_str());

            if (depthInfoJson.FindMember("bids") == depthInfoJson.MemberEnd())
            {
                cout << __func__ << " " << __LINE__ << " no depth data, return" << endl;
                return;
            }

            uint64_t lastUpdateId = depthInfoJson["lastUpdateId"].GetUint64();

            const auto &bids = depthInfoJson["bids"];
            const auto &asks = depthInfoJson["asks"];
        

            if (bids.Size() < 0 || asks.Size() < 0 || bids[0].Size() < 2 || asks[0].Size() < 0)
            {
                cout << __func__ << " " << __LINE__ << "error ! depth data error!" << msg->get_payload().c_str() << endl;
                return;
            }

            double bidPrice = 0, bidValue = 0;
            double askPrice = 0, askValue = 0;
            
            //构建请求结构体
            NotifyDepthReq req;
            
            for (unsigned i = 0; i < bids.Size(); i++)
            {
                const auto &bid = bids[i];
                // const auto &bid = bids[0];
                std::string strBidPrice = bid[0].GetString();
                std::string strBidValue = bid[1].GetString();
                websocketclient::websocketclient::String2Double(strBidPrice.c_str(), bidPrice);
                websocketclient::websocketclient::String2Double(strBidValue.c_str(), bidValue);
                req.Bids[i].price =  bidPrice;
                req.Bids[i].quantity, bidValue;
            }
            for (unsigned i = 0; i < bids.Size(); i++)
            {
                const auto &ask = asks[i];
                std::string strAskPrice = ask[0].GetString();
                std::string strAskValue = ask[1].GetString();
                websocketclient::websocketclient::String2Double(strAskPrice.c_str(), askPrice);
                websocketclient::websocketclient::String2Double(strAskValue.c_str(), askValue);
                req.Asks[i].price, askPrice;
                req.Asks[i].quantity, askValue;
            }
            uint64_t updateTime = getTime();
            req.UpdateTime = updateTime;

        };

        //添加depth订阅
        void depthwebsocketclient::AddDepthSubscirbe(std::string exchangeName)
        {
            if (depth_ws_client.count(exchangeName))
            {
                cout << __func__ << " " << __LINE__ << exchangeName << " already connected" << endl;
                return;
            }

            std::string msg = R"({"method":"SUBSCRIBE","params":[")" + exchangeName + R"(@depth5@100ms"],"id":)" + to_string(time(NULL) % 1000) + R"(})";
            depthwebsocketclient *ws_client = new depthwebsocketclient(exchangeName, ioService, msg);
            ws_client->Connect(bind(&depthwebsocketclient::DepthMsgHandler, this, ::_1, ::_2, exchangeName), "");
            depth_ws_client[exchangeName] = ws_client;
        };
