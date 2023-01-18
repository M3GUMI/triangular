#include <sstream>

#include "triangular.h"
#include "lib/binance/binancecommondef.h"

using namespace binancecommondef;

namespace Triangular
{
    void Triangular::CreateAndSubListenKey()
    {
        std::string uri = "https://api.binance.com/api/v3/userDataStream";
        restapi_client->BinanceMakeRequest({}, "POST", uri, "", std::bind(&Triangular::CreateListkeyHandler, this, ::_1, ::_2), true);
    }

    void Triangular::CreateListkeyHandler(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec)
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
            m_listenKey = listenKeyInfo.FindMember("listenKey")->value.GetString();
            cout << __func__ << " " << __LINE__ << "get listen key " << m_listenKey << endl;
            ws_client = new BinanceWebsocketWrapper::BinanceWebsocketWrapper("", &m_ioService, "");
            ws_client->Connect(bind(&Triangular::WSMsgHandler, this, ::_1, ::_2), string("/") + m_listenKey);
        }

        listenkeyKeepTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(m_ioService, websocketpp::lib::asio::milliseconds(next_keep_time_ms));
        listenkeyKeepTimer->async_wait(std::bind(&Triangular::listenkeyKeepHandler, this));
    }

    void Triangular::listenkeyKeepHandler()
    {
        cout << __func__ << " " << __LINE__ << "Begin keep listenkey" << endl;
        map<std::string, std::string> args;
        args["listenKey"] = m_listenKey;
        std::string uri = "https://api.binance.com/api/v3/userDataStream";
        restapi_client->BinanceMakeRequest(args, "PUT", uri, "", std::bind(&Triangular::CreateListkeyHandler, this, ::_1, ::_2), false);
    }

    void Triangular::WSMsgHandler(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg)
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

    static void String2Double(const string &str, double &d)
    {
        std::istringstream s(str);
        s >> d;
    }

    void Triangular::ExecutionReportHandler(const rapidjson::Document &msg)
    {
        std::string symbol = msg.FindMember("s")->value.GetString();
        std::string from, to;
        std::string side = msg.FindMember("S")->value.GetString();
        std::string ori = msg.FindMember("q")->value.GetString();
        std::string exced = msg.FindMember("z")->value.GetString();
        double doubleExced = 0, doubleOri = 0;
        String2Double(exced, doubleExced);
        String2Double(ori, doubleOri);
        std::string status = msg.FindMember("X")->value.GetString();

        from = m_mapSymbol2Base[symbol].first;
        to = m_mapSymbol2Base[symbol].second;

        cout << __func__ << " " << __LINE__ << " " << symbol << " ExecutionReportHandler " << side << " " << ori << " " << exced << endl;

        if (side == "BUY")
        {
            swap(from, to);
        }

        if (IsStatic(from) && IsStatic(to))
        {
            // 啥也不干 更新一下订单状态
            if (status == "FILLED")
            {
                mapSymbolBalanceOrderStatus[symbol] = 0;
            }
        }

        // 稳定币到x币 ioc 直接用成交的部分跑 x到稳定币
        else if (IsStatic(from) && not IsStatic(to))
        {
            // 如果有成交的部分 把成交的部分挂 x币到稳定币
            if (doubleExced != 0)
            {
                auto path = TriangularBestPathFinder(to, from);
                uint32_t from_index = path[path.size() - 1], to_index = path[path.size() - 2];
                std::string begin_coin = m_mapIndex2Coins[from_index], next_coin = m_mapIndex2Coins[to_index];
                TriangularCreateOrder(begin_coin, next_coin, doubleExced);
            }
        }

        // 从x币到稳定币 ioc 尽可能全部交易完
        else if (not IsStatic(from))
        {
            // 如果没有完全成交 再把未完全成交的部分挂出去
            if (status != "FILLED")
            {
                TriangularCreateOrder(from, to, doubleOri - doubleExced);
            }
            else if (status == "FILLED")// 如果交易完全成交 开始平衡仓位
            {
                // GetBalanceAndCheck();
            }
        }
    }
}