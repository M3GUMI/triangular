#include <sstream>

#include "triangular.h"
#include "lib/binance/binancecommondef.h"

using namespace binancecommondef;

namespace Triangular
{

    static void String2Double(const string &str, double &d)
    {
        std::istringstream s(str);
        s >> d;
    }

    void Triangular::TryRebalanceOrderHandler(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec)
    {
        if (res == nullptr || res->payload().empty())
        {
            cout << __func__ << " " << __LINE__ << " try balance error " << endl;
            return;
        }

        auto &msg = res->payload();
        if (res->http_status() != 200)
        {
            cout << __func__ << " " << __LINE__ << "***** try balance http status error *****" << res->http_status() << " msg: " << msg << endl;
            return;
        }

        rapidjson::Document order;
        order.Parse(msg.c_str());

        // cout << __func__ << " " << __LINE__ << msg.c_str() << endl;
        std::string symbol = order.FindMember("symbol")->value.GetString();
        uint64_t orderid = order.FindMember("orderId")->value.GetUint64();
        std::string clientOrderId = order.FindMember("clientOrderId")->value.GetString();

        mapSymbolBalanceOrderStatus[symbol] = 1;
    }

    void Triangular::GetBalanceAndCheck()
    {
        string requestLink = "https://api.binance.com/api/v3/account";
        map<std::string, std::string> args;
        uint64_t now = time(NULL);
        args["timestamp"] = to_string(now * 1000);
        restapi_client->BinanceMakeRequest(args, "GET", requestLink, "", std::bind(&Triangular::AccountInfoHandler, this, ::_1, ::_2), true);

        balanceTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(m_ioService, websocketpp::lib::asio::milliseconds(5 * 60 * 1000));
        balanceTimer->async_wait(std::bind(&Triangular::GetBalanceAndCheck, this));
    }

    void Triangular::GetBalanceAndCheckNoWait()
    {
        string requestLink = "https://api.binance.com/api/v3/account";
        map<std::string, std::string> args;
        uint64_t now = time(NULL);
        args["timestamp"] = to_string(now * 1000);
        restapi_client->BinanceMakeRequest(args, "GET", requestLink, "", std::bind(&Triangular::AccountInfoHandler, this, ::_1, ::_2), true);
    }

    void Triangular::AccountInfoHandler(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec)
    {
        if (res == nullptr || res->payload().empty())
        {
            cout << __func__ << " " << __LINE__ << " get_balance error " << endl;
            return;
        }

        auto &msg = res->payload();
        if (res->http_status() != 200)
        {
            cout << __func__ << " " << __LINE__ << "***** get_balance http status error *****" << res->http_status() << " msg: " << msg << endl;
            return;
        }

        rapidjson::Document accountInfoDocument;
        accountInfoDocument.Parse(msg.c_str());
        if (not accountInfoDocument.HasMember("balances"))
        {
            cout << __func__ << " " << __LINE__ << "no balance data " << msg.c_str() << endl;
            return;
        }

        auto balances = accountInfoDocument["balances"].GetArray();
        map<std::string, double> mapCoins2Free;
        for (int i = 0; i < balances.Size(); i++)
        {
            std::string asset = balances[i].FindMember("asset")->value.GetString();
            std::string free = balances[i].FindMember("free")->value.GetString();
            double doubleFree = 0;
            String2Double(free, doubleFree);

            mapCoins2Free[asset] = doubleFree;
        }

        std::string coin_need_add;
        std::string coin_can_del;
        double maxPercent = 0;

        for (const auto &item : mapCoins2Free)
        {
            auto coin = item.first;
            auto free = item.second;

            if (not m_mapCoins2BeginAmount.count(coin))
            {
                continue;
            }

            if (free < m_mapCoins2BeginAmount[coin])
            {
                coin_need_add = coin;
            }

            if (free > m_mapCoins2BeginAmount[coin] && free / m_mapCoins2BeginAmount[coin] > maxPercent)
            {
                coin_can_del = coin;
                maxPercent = free / m_mapCoins2BeginAmount[coin];
            }

            if (not coin_need_add.empty() && not coin_can_del.empty())
            {
                break;
            }
        }

#define min(x, y) (x > y ? y : x)

        if (not coin_need_add.empty() && not coin_can_del.empty())
        {
            TryRebalance(coin_need_add, coin_can_del, mapCoins2Free[coin_can_del] - m_mapCoins2BeginAmount[coin_can_del]);
        }
    }

    void Triangular::TryRebalance(std::string to, std::string from, double amount)
    {
        cout << __func__ << " " << __LINE__ << "begin balance " << from << " " << to << " " << amount << endl;

        // uint32_t orderType = Order_Type::LIMIT;
        uint32_t orderType = Order_Type::LIMIT_MAKER;
        uint32_t orderSide = Order_Side::BUY;
        uint32_t timeInForce = TimeInFoce::IOC;
        std::string symbol;
        double price = 0;
        uint32_t from_index = m_mapCoins2Index[from];
        uint32_t to_index = m_mapCoins2Index[to];
        for (const auto &item : m_gExchangeMap[from_index])
        {
            if (item.to_point == to_index)
            {
                orderSide = item.side;
                price = item.price;
            }
        }

        if (orderSide == Order_Side::BUY)
        {
            symbol = to + from;
            price = price;
            amount = amount / price;
        }
        else
        {
            symbol = from + to;
        }

        double ticketSize = (m_mapExchange2Ticksize[symbol] == 0 ? 1 : m_mapExchange2Ticksize[symbol]);
        uint32_t tmp = amount / ticketSize;
        amount = tmp * ticketSize;
        if (amount <= 0)
        {
            return;
        }

        restapi_client->CreateOrder(symbol, orderSide, orderType, timeInForce, amount, price, bind(&Triangular::TryRebalanceOrderHandler, this, ::_1, ::_2));
    }

    void Triangular::CancelAllCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec, std::string ori_symbol)
    {
        if (res == nullptr || res->payload().empty())
        {
            cout << __func__ << " " << __LINE__ << " cancel all error " << endl;
            return;
        }

        auto &msg = res->payload();
        rapidjson::Document order;
        order.Parse(msg.c_str());

        if (res->http_status() != 200)
        {
            cout << __func__ << " " << __LINE__ << "***** cancel all http status error *****" << res->http_status() << " msg: " << msg << endl;
            int code = order.FindMember("code")->value.GetInt();
            std::string msg = order.FindMember("msg")->value.GetString();

            // 找不到订单了
            if (code == -2011 && msg == "Unknown order sent.")
            {
                mapSymbolBalanceOrderStatus[ori_symbol] = 0;
            }
            return;
        }

        cout << __func__ << " " << __LINE__ << msg.c_str() << endl;

        // cout << __func__ << " " << __LINE__ << msg.c_str() << endl;
        // std::string symbol = order.FindMember("symbol")->value.GetString();

        mapSymbolBalanceOrderStatus[ori_symbol] = 0;
    }

    void Triangular::CancelAllOrder(std::string symbol)
    {
        map<string, string> args;
        args["symbol"] = symbol;
        args["timestamp"] = std::to_string(time(NULL) * 1000);
        args["recvWindow"] = "50000";
        std::string uri = "https://api.binance.com/api/v3/openOrders";
        restapi_client->BinanceMakeRequest(args, "DELETE", uri, "", std::bind(&Triangular::CancelAllCallback, this, ::_1, ::_2, symbol), true);
    }
}