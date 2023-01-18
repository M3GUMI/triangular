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

    void Triangular::UpdateExhangeMapOneStep(unsigned from, unsigned to, double price, double fee, uint64_t lastUpdateId, uint32_t side, bool &is_price_change)
    {
        if (from >= m_gExchangeMap.size())
        {
            return;
        }
        for (unsigned i = 0; i < m_gExchangeMap[from].size(); i++)
        {
            auto x = m_gExchangeMap[from][i];
            if (x.to_point == to)
            {
                if (lastUpdateId <= x.lastUpdateId)
                {
                    return;
                }

                if (x.price != price)
                {
                    is_price_change = true;
                }
                m_gExchangeMap[from][i] = TriangularExchangeMapNode(to, price, fee, lastUpdateId, side);
                return;
            }
        }

        is_price_change = true;
        m_gExchangeMap[from].push_back(TriangularExchangeMapNode(to, price, fee, lastUpdateId, side));
    }

    void Triangular::UpdateExhangeMap(string symbol, double bid, double ask, double fee, uint64_t lastUpdateId, bool &is_price_change)
    {
        if (not m_mapSymbol2Base.count(symbol))
        {
            cout << __func__ << " " << __LINE__ << "symbol not found " << symbol << endl;
            return;
        }

        if (bid == 0 || ask == 0)
        {
            cout << __func__ << " " << __LINE__ << "bid or ask = 0 " << bid << " " << ask << endl;
            return;
        }

        // cout << __func__ << " " << __LINE__ << "update " << symbol << " " << bid << " " << ask << " " << fee << endl;

        std::string baseAsset = m_mapSymbol2Base[symbol].first;
        std::string quoteAsset = m_mapSymbol2Base[symbol].second;

        unsigned baseAssetIndex = m_mapCoins2Index[baseAsset];
        unsigned quoteAssetIndex = m_mapCoins2Index[quoteAsset];

        UpdateExhangeMapOneStep(baseAssetIndex, quoteAssetIndex, bid, fee, lastUpdateId, Order_Side::SELL, is_price_change);
        UpdateExhangeMapOneStep(quoteAssetIndex, baseAssetIndex, ask, fee, lastUpdateId, Order_Side::BUY, is_price_change);
    }

    string toupper(const string &str)
    {
        string s = str;
        transform(s.begin(), s.end(), s.begin(), ::toupper);
        return s;
    }

    void Triangular::OrderHandler(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec)
    {
        if (res == nullptr || res->payload().empty())
        {
            cout << __func__ << " " << __LINE__ << " get_balance error " << endl;
            return;
        }

        auto &msg = res->payload();
        rapidjson::Document order;
        order.Parse(msg.c_str());
        if (res->http_status() != 200)
        {
            cout << __func__ << " " << __LINE__ << "***** get_balance http status error *****" << res->http_status() << " msg: " << msg << endl;
            int code = order.FindMember("code")->value.GetInt();
            std::string msg = order.FindMember("msg")->value.GetString();
            if (code == -2010 && msg == "Account has insufficient balance for requested action.")
            {
                GetBalanceAndCheckNoWait();
            }
            return;
        }

        cout << __func__ << " " << __LINE__ << msg.c_str() << endl;

        return;

        // std::string symbol = order["symbol"].GetString();
        // uint64_t orderid = order["orderId"].GetUint64();
        // std::string clientOrderId = order["clientOrderId"].GetString();
        // std::string executedQty = order["executedQty"].GetString();
        // std::string origQty = order["origQty"].GetString();
        // std::string side = order["side"].GetString();

        // std::string now, other;
        // if (side == "SELL")
        // {
        //     now = m_mapSymbol2Base[symbol].second;
        //     other = m_mapSymbol2Base[symbol].first;
        // }
        // else
        // {
        //     now = m_mapSymbol2Base[symbol].first;
        //     other = m_mapSymbol2Base[symbol].second;
        // }

        // double executed = 0;
        // double orig = 0;
        // String2Double(executedQty, executed);
        // String2Double(origQty, orig);

        // cout << __func__ << " " << __LINE__ << "order callback "
        //      << " " << now << " " << other << " " << executed << endl;

        // if (IsStatic(now))
        // {
        //     // todo rebalance
        // }

        // if (executed != 0 && not IsStatic(now))
        // {
        //     auto path = TriangularBestPathFinder(now, other);
        //     assert(path.size() >= 2);

        //     uint32_t from = path[path.size() - 1], to = path[path.size() - 2];
        //     std::string begin_coin = m_mapIndex2Coins[from], next_coin = m_mapIndex2Coins[to];
        //     TriangularCreateOrder(begin_coin, next_coin, executed);
        // }

        // // 未完全成交
        // if (orig - executed > 0)
        // {
        //     TriangularCreateOrder(other, now, orig - executed);
        // }
    }

    void Triangular::DepthMsgHandler(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg, std::string exchangeName, bool b_run_path_cal)
    {
        // cout << __func__ << " " << __LINE__ << msg->get_payload().c_str() << endl;
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
        // for (unsigned i = 0; i < bids.Size(); i++)
        {
            // const auto &bid = bids[i];
            const auto &bid = bids[0];
            std::string strBidPrice = bid[0].GetString();
            std::string strBidValue = bid[1].GetString();
            String2Double(strBidPrice.c_str(), bidPrice);
            String2Double(strBidValue.c_str(), bidValue);
        }

        {
            const auto &ask = asks[0];
            std::string strAskPrice = ask[0].GetString();
            std::string strAskValue = ask[1].GetString();
            String2Double(strAskPrice.c_str(), askPrice);
            String2Double(strAskValue.c_str(), askValue);
        }

        exchangeName = toupper(exchangeName);

        double fee = 0.0 / 10000;
        if (IsStatic(m_mapSymbol2Base[exchangeName].first) && IsStatic(m_mapSymbol2Base[exchangeName].second))
        {
            fee = 0;
        }

        bool is_price_change = false;
        UpdateExhangeMap(exchangeName, bidPrice, askPrice, fee, lastUpdateId, is_price_change);

        if (is_price_change && mapSymbolBalanceOrderStatus[exchangeName] == 1)
        {
            CancelAllOrder(exchangeName);
        }

        // if (m_signalTrade >= 3)
        // {
        //     // ShowPath(path);
        //     cout << __func__ << " " << __LINE__ << " m_signalTrade exit " << endl;
        //     exit(0);
        // }

        if (b_run_path_cal && time(NULL) - init_time > 10)
        {
            auto paths = TriangularKiller(m_mapSymbol2Base[exchangeName].second, m_mapSymbol2Base[exchangeName].second);
            print_counter++;
            if (print_counter % 100 == 0)
            {
                cout << __func__ << " " << __LINE__ << "calc finish, find path " << paths.size() << endl;
                cout << __func__ << " " << __LINE__ << "[counter:" << print_counter << "]" << endl;
            }

            for (const auto &path : paths)
            {
                m_signalTrade++;

                if (path.size() <= 2)
                {
                    return;
                }

                uint32_t pos = path.size() - 1;
                uint32_t begin = path[pos];
                uint32_t next = path[pos - 1];
                std::string begin_coin = m_mapIndex2Coins[begin], next_coin = m_mapIndex2Coins[next];

                while (IsStatic(next_coin) && pos >= 2)
                {
                    pos--;
                    begin = path[pos];
                    next = path[pos - 1];
                    begin_coin = m_mapIndex2Coins[begin];
                    next_coin = m_mapIndex2Coins[next];
                }

                if (IsStatic(next_coin))
                {
                    cout << __func__ << " " << __LINE__ << "static coin, something error " << next_coin << endl;
                    ShowPath(path);
                    continue;
                }

                double begin_amount = m_mapCoins2BeginAmount[begin_coin];

                cout << __func__ << " " << __LINE__ << "try create order " << begin_coin << " " << next_coin << " " << begin_amount << endl;
                TriangularCreateOrder(begin_coin, next_coin, begin_amount);
                break;
            }
        }
    }

    void Triangular::ShowPath(vector<uint32_t> path)
    {

        cout << __func__ << " " << __LINE__ << endl;
        for (const auto &p : path)
        {
            cout << "[" << p << " " << m_mapIndex2Coins[p] << "] ";
        }

        cout << endl;

        double now_mon = 1;
        for (int i = path.size() - 1; i > 0; i--)
        {
            uint32_t index = path[i];
            uint32_t next = path[i - 1];
            std::string coin = m_mapIndex2Coins[index];
            std::string next_coin = m_mapIndex2Coins[next];

            double fee = 0, price = 0;
            uint32_t side = 0;
            for (const auto &item : m_gExchangeMap[index])
            {
                if (item.to_point == next)
                {
                    fee = item.fee;
                    price = item.price;
                    side = item.side;
                    break;
                }
            }

            double new_account = 0;
            if (side == Order_Side::SELL)
            {
                new_account =  now_mon * price * (1 - fee);
            }
            else
            {
                new_account = now_mon / price * (1 - fee);
            }
            printf("%lf %s ~ %lf %s [price:%lf fee:%lf]\n", now_mon, coin.c_str(), new_account, next_coin.c_str(), price, fee);
            now_mon = new_account;
        }

        cout << __func__ << " " << __LINE__ << endl;
    }

    vector<uint32_t> Triangular::GetTriangularKillerPath(uint32_t start, uint32_t end, std::map<uint32_t, uint32_t> &mapIndex2PerPath)
    {
        map<uint32_t, bool> mapUsedNode;
        vector<uint32_t> vecPath;

        uint32_t now = end;
        do
        {
            vecPath.push_back(now);
            mapUsedNode[now] = true;
            now = mapIndex2PerPath[now];
        } while (now != start && not mapUsedNode[now]);
        vecPath.insert(vecPath.begin(), *(vecPath.end() - 1));
        return vecPath;
    }

    vector<uint32_t> Triangular::GetBestPath(uint32_t start, uint32_t end, std::map<uint32_t, uint32_t> &mapIndex2PerPath)
    {
        map<uint32_t, bool> mapUsedNode;
        vector<uint32_t> vecPath;

        uint32_t now = end;
        do
        {
            vecPath.push_back(now);
            mapUsedNode[now] = true;
            now = mapIndex2PerPath[now];
        } while (now != start && not mapUsedNode[now]);
        vecPath.push_back(start);

        return vecPath;
    }

    vector<vector<uint32_t>> Triangular::TriangularKiller(std::string begin, std::string end)
    {
        std::map<uint32_t, double> mapIndex2MaxAccount;
        std::map<uint32_t, uint32_t> mapIndex2PerPath;
        std::map<uint32_t, bool> mapIndexUsed;

        queue<pair<uint32_t, uint32_t>> que;
        vector<vector<uint32_t>> vecPaths;

        uint32_t begin_index = m_mapCoins2Index[begin];
        uint32_t end_index = m_mapCoins2Index[end];

        if (begin_index >= m_gExchangeMap.size())
        {
            return vecPaths;
        }

        // mapIndex2MaxAccount[begin_index] = 1;
        // mapIndex2PerPath[begin_index] = -1;
        // mapIndexUsed[begin_index] = true;

        for (const auto &item : m_gExchangeMap[begin_index])
        {
            uint32_t to = item.to_point;
            double price = item.price;
            double fee = item.fee;
            uint32_t side = item.side;

            double new_account = 0;
            if (side == Order_Side::SELL)
            {
                new_account = 1 * price * (1 - fee);
            }
            else
            {
                new_account = 1 / price * (1 - fee);
            }

            mapIndex2MaxAccount[to] = new_account;
            mapIndex2PerPath[to] = begin_index;
            mapIndexUsed[to] = true;
            que.push(make_pair(to, 1));
        }

        while (not que.empty())
        {
            uint32_t now = que.front().first;
            uint32_t depth = que.front().second;
            que.pop();
            mapIndexUsed[now] = false;

            if (depth >= 3)
                continue;

            // 走到终点了
            if (now == end_index)
            {
                // 没利润
                if (mapIndex2MaxAccount[now] < 1.0001)
                {
                    // cout << __func__ << " " << __LINE__ << "end but no perfit " << now << " " << mapIndex2MaxAccount[now] << endl;
                    continue;
                }

                auto path = GetTriangularKillerPath(begin_index, now, mapIndex2PerPath);
                vecPaths.push_back(path);
                continue;
            }

            // 需要继续走
            for (const auto &item : m_gExchangeMap[now])
            {
                uint32_t to = item.to_point;
                double price = item.price;
                double fee = item.fee;

                double new_account = 0;
                if (item.side == Order_Side::SELL)
                {
                    new_account =  mapIndex2MaxAccount[now] * price * (1 - fee);
                }
                else
                {
                    new_account =  mapIndex2MaxAccount[now] / price * (1 - fee);
                }

                if (mapIndex2MaxAccount[to] < new_account)
                {
                    mapIndex2MaxAccount[to] = new_account;
                    mapIndex2PerPath[to] = now;
                    if (not mapIndexUsed[to])
                    {
                        mapIndexUsed[to] = true;
                        que.push(make_pair(to, depth + 1));
                    }
                }
            }
        }

        return vecPaths;
    }

    vector<uint32_t> Triangular::TriangularBestPathFinder(std::string begin, std::string end)
    {
        std::map<uint32_t, double> mapIndex2MaxAccount;
        std::map<uint32_t, uint32_t> mapIndex2PerPath;
        std::map<uint32_t, bool> mapIndexUsed;

        vector<uint32_t> vecPath;
        queue<pair<uint32_t, uint32_t>> que;
        uint32_t begin_index = m_mapCoins2Index[begin];
        uint32_t end_index = m_mapCoins2Index[end];

        if (begin_index >= m_gExchangeMap.size())
        {
            cout << __func__ << " " << __LINE__ << "index error" << endl;
            return vecPath;
        }

        double maxAmount = -1;
        que.push(make_pair(begin_index, 0));
        mapIndex2MaxAccount[begin_index] = 1;
        mapIndex2PerPath[begin_index] = -1;
        mapIndexUsed[begin_index] = true;

        while (!que.empty())
        {
            uint32_t now = que.front().first;
            uint32_t depth = que.front().second;
            que.pop();
            mapIndexUsed[now] = false;

            if (now == end_index)
            {
                if (mapIndex2MaxAccount[now] > maxAmount)
                {
                    maxAmount = mapIndex2MaxAccount[now];
                    vecPath = GetBestPath(begin_index, end_index, mapIndex2PerPath);
                }
            }

            if (depth > 3)
            {
                continue;
            }

            for (const auto &item : m_gExchangeMap[now])
            {
                uint32_t to = item.to_point;
                double price = item.price;
                double fee = item.fee;

                if (mapIndex2MaxAccount[to] < mapIndex2MaxAccount[now] * price * (1 - fee))
                {
                    mapIndex2MaxAccount[to] = mapIndex2MaxAccount[now] * price * (1 - fee);
                    mapIndex2PerPath[to] = now;
                    if (not mapIndexUsed[to])
                    {
                        mapIndexUsed[to] = true;
                        que.push(make_pair(to, depth + 1));
                    }
                }
            }
        }

        return vecPath;
    }

    void Triangular::AddDepthSubscirbe(std::string exchangeName, bool b_run_path_cal)
    {
        if (depth_ws_client.count(exchangeName))
        {
            cout << __func__ << " " << __LINE__ << exchangeName << " already connected" << endl;
            return;
        }

        std::string msg = R"({"method":"SUBSCRIBE","params":[")" + exchangeName + R"(@depth5@100ms"],"id":)" + to_string(time(NULL) % 1000) + R"(})";
        BinanceWebsocketWrapper::BinanceWebsocketWrapper *ws_client = new BinanceWebsocketWrapper::BinanceWebsocketWrapper(exchangeName, &m_ioService, msg);
        ws_client->Connect(bind(&Triangular::DepthMsgHandler, this, ::_1, ::_2, exchangeName, b_run_path_cal), "");
        depth_ws_client[exchangeName] = ws_client;
    }

    void Triangular::TriangularCreateOrder(std::string from, std::string to, double amount)
    {
        if (amount <= 0)
        {
            return;
        }

        uint32_t begin = m_mapCoins2Index[from];
        uint32_t next = m_mapCoins2Index[to];
        cout << __func__ << " " << __LINE__ << "from ~ to"
             << " " << from << " " << to << " " << begin << " " << next << endl;
        uint32_t side = Order_Side::BUY;
        double price = 0;
        std::string symbol;
        for (const auto &item : m_gExchangeMap[begin])
        {
            // cout << __func__ << " " << __LINE__ << "item " << item.to_point << " " << item.side << " " << item.price << endl;
            if (item.to_point == next)
            {
                side = item.side;
                if (side == Order_Side::BUY)
                {
                    price = item.price;
                    symbol = to + from;
                    amount = amount / price;
                }
                else
                {
                    price = item.price;
                    symbol = from + to;
                }

                break;
            }
        }

        double ticketSize = (m_mapExchange2Ticksize[symbol] == 0 ? 1 : m_mapExchange2Ticksize[symbol]);

        uint32_t tmp = amount / ticketSize;
        amount = tmp * ticketSize;

        if (amount <= 0)
        {
            return;
        }

        cout << __func__ << " " << __LINE__ << "create order " << from << " " << to << " " << symbol << " " << side << " " << amount << " " << price << endl;
        cout << __func__ << " " << __LINE__ << "tick size" << m_mapExchange2Ticksize[symbol] << endl;

        uint32_t orderType = Order_Type::LIMIT;
        uint32_t timeInForce = TimeInFoce::IOC;

        restapi_client->CreateOrder(symbol, side, orderType, timeInForce, amount, price, bind(&Triangular::OrderHandler, this, ::_1, ::_2));
    }

    bool Triangular::IsStatic(std::string coin)
    {
        return m_mapCoins2BeginAmount.count(coin);
    }
}