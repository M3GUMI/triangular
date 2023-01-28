#include "binancewebrestapiwrapper.h"

using namespace webrestapiwrapper;

binancewebrestapiwrapper::binancewebrestapiwrapper(websocketpp::lib::asio::io_service &ioService, std::string accessKey, std::string secretKey) : ioService(&ioService), accessKey(accessKey), secretKey(secretKey)
{
}
binancewebrestapiwrapper::binancewebrestapiwrapper()
{
}
binancewebrestapiwrapper::~binancewebrestapiwrapper()
{
}

void binancewebrestapiwrapper::binanceInitSymbol()
{
    string requestLink = "https://api.binance.com/api/v3/exchangeInfo";
    MakeRequest({}, "GET", requestLink, "", std::bind(&ExchangeInfoHandler, this, ::_1, ::_2), false);
    init_time = time(NULL);
}
void binancewebrestapiwrapper::ExchangeInfoHandler(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec)
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

    rapidjson::Document exchangeInfoJson;
    exchangeInfoJson.Parse(msg.c_str());
    if (exchangeInfoJson.HasMember("symbols"))
    {
        const auto &symbols = exchangeInfoJson["symbols"];
        for (unsigned i = 0; i < symbols.Size(); ++i)
        {
            std::string symbol = symbols[i].FindMember("symbol")->value.GetString();
            std::string baseAsset = symbols[i].FindMember("baseAsset")->value.GetString();
            std::string quoteAsset = symbols[i].FindMember("quoteAsset")->value.GetString();
            if (baseAsset == "NGN" || quoteAsset == "NGN")
            {
                continue;
            }
            symbolMap[make_pair(baseAsset, quoteAsset)] = symbol;
            symbolMap[make_pair(quoteAsset, baseAsset)] = symbol;
            baseOfSymbol[symbol] = baseAsset;
            m_mapSymbol2Base[symbol] = make_pair(baseAsset, quoteAsset);
            m_setBaseCoins.insert(baseAsset);
            m_setBaseCoins.insert(quoteAsset);

            if (not symbols[i].HasMember("filters"))
            {
                continue;
            }

            auto filters = symbols[i].FindMember("filters")->value.GetArray();
            for (uint32_t j = 0; j < filters.Size(); j++)
            {
                if (not filters[j].HasMember("stepSize") || not filters[j].HasMember("filterType"))
                {
                    continue;
                }

                std::string filterType = filters[j].FindMember("filterType")->value.GetString();

                if (filterType != "LOT_SIZE")
                {
                    continue;
                }

                std::string ticksize = filters[j].FindMember("stepSize")->value.GetString();
                double double_tickSize = 0;
                String2Double(ticksize, double_tickSize);
                m_mapExchange2Ticksize[symbol] = double_tickSize;
                break;
            }
        }
    }
}

void binancewebrestapiwrapper::MakeRequest(map<string, string> args, string method, string uri, string data, std::function<void(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec)> func, bool need_sign)
{

    string query;
    const auto &header = sign(args, query, need_sign);

    std::string whole_uri;
    if (query.empty())
    {
        whole_uri = uri;
    }
    else
    {
        whole_uri = uri + "?" + query;
    }

    cout << "whole uri = " << whole_uri << endl;

    auto req = HttpRequest::make_request(method, whole_uri, data);
    req->Header = header;
    hclientPtr->Do(req, func);

    cout << "End request" << endl;
}

std::map<std::string, std::string> binancewebrestapiwrapper::sign(std::map<std::string, std::string> &args, std::string &query, bool need_sign)
{
    std::map<std::string, std::string> header;
    header["X-MBX-APIKEY"] = accessKey;

    if (not args.empty())
    {
        query = toURI(args);
        if (need_sign)
        {
            args["signature"] = hmac(secretKey, query, EVP_sha256(), Strings::hex_to_string);
            query = toURI(args);
        }
    }

    return header;
}

std::string binancewebrestapiwrapper::toURI(const map<std::string, std::string> &mp)
{   
   
    string str;
    for (auto it = mp.begin(); it != mp.end(); it++)
    {
        str += it->first + "=" + Strings::uri_escape_string(it->second) + "&";
    }
    return str.empty() ? str : str.erase(str.size() - 1);
}

std::string binancewebrestapiwrapper::BcreateOrder(CreateOrderReq sArgs)
{
    createOrder(sArgs, OrderHandler);
}
template <typename T, typename outer>
std::string binancewebrestapiwrapper::hmac(const std::string &key, const std::string &data, T evp, outer filter)
{
    CreateOrderReq a;
    createOrder(a, OrderHandler);

    unsigned char mac[EVP_MAX_MD_SIZE] = {};
    unsigned int mac_length = 0;

    HMAC(evp, key.data(), int(key.size()),
         (unsigned char *)data.data(), data.size(),
         mac, &mac_length);
    /*HMAC_CTX ctx;
    HMAC_CTX_new(&ctx);
    HMAC_Init_ex(&ctx, key.data(), int(key.size()), evp, NULL);
    HMAC_Update(&ctx, (unsigned char *)data.data(), data.size());
    HMAC_Final(&ctx, mac, &mac_length);
    HMAC_CTX_reset(&ctx);*/

    return filter(mac, mac_length);
}

void binancewebrestapiwrapper::createOrder(CreateOrderReq sArgs, std::function<void(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec)> func)
{
    std::string uri = "https://api.binance.com/api/v3/order";
    map<string, string> args;
    pair symbolSize = getSymbolSize(sArgs.FromToken, sArgs.ToToken);
    args["symbol"] = symbolSize.first;
    args["timestamp"] = std::to_string(getTime());
    args["side"] = symbolSize.second;
    args["type"] = sArgs.OrderType;
    args["price"] = selectPriceQuantity(sArgs, args["symvol"]).first;
    args["quantity"] = selectPriceQuantity(sArgs, args["symbol"]).second;
    args["timeInForce"] = sArgs.TimeInForce;
    MakeRequest(args, "POST", uri, "", func, true);
}

// 发起取消订单方法
void binancewebrestapiwrapper::cancelOrder(std::string symbol, std::string clientOrderId)
{
    map<string, string> args;
    args["symbol"] = symbol;
    args["timestamp"] = std::to_string(time(NULL) * 1000);
    args["recvWindow"] = "50000";
    args["newClientOrderId"] = clientOrderId;
    std::string uri = "https://api.binance.com/api/v3/openOrders";
    MakeRequest(args, "DELETE", uri, "", std::bind(&binancewebrestapiwrapper::CancelAllCallback, this, ::_1, ::_2, symbol), true);
}

void binancewebrestapiwrapper::CancelAllCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec, std::string ori_symbol)
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
pair<std::string, std::string> getSymbolSize(std::string token0, std::string token1)
{
    pair<std::string, std::string> res;
    return res;
}

void binancewebrestapiwrapper::OrderHandler(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec)
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
            cout << "余额不足" << endl;
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

pair<std::string, std::string> binancewebrestapiwrapper::getSymbolSize(std::string token0, std::string token1)
{
    if (symbolMap.count(make_pair(token0, token1)) == 1)
    {
        std::string symbol = symbolMap[make_pair(token0, token1)];
        std::string base = baseOfSymbol[symbol];
        std::string side;
        base == token0 ? side = "SELL" : side = "BUY";
        cout << "symbol : " << symbol << "side : " << side << endl;
        return make_pair(symbol, side);
    }
    cout << "not found error" << endl;
    return make_pair("null", "null");
}

pair<double, double> binancewebrestapiwrapper::selectPriceQuantity(CreateOrderReq args, std::string symbol)
{
    std::string base = baseOfSymbol[symbol];
    double price, quantity;
    base == args.FromToken ? price = args.FromPrice, quantity = args.FromQuantity : price = args.ToPrice, quantity = args.ToQuantity;
    return make_pair(price, quantity);
}

 void binancewebrestapiwrapper::BcancelOrder(cancelOrderReq cArgs)
 {
    bool isCancelAll = cArgs.CancelAll;
    uint64_t clientOrderId = cArgs.OrderId;
    vector<std::string> symbols;
    for(int i = 0;i <= cArgs.pairs.size(); i ++ ){
        pair pair = cArgs.pairs[i];
        std::string symbol = getSymbolSize(pair.first, pair.second).first;
        symbols.push_back(symbol);
    }
    if(isCancelAll == true){
        for(auto it = symbolMap.begin(); it != symbolMap.end(); )
        {
            cancelOrder(it -> second, to_string(clientOrderId));
            it = 2 + it;
        }
    }else
    {
        for(auto i = 0; i <= symbols.size(); i++)
        {
            cancelOrder(symbols[i], to_string(clientOrderId));
        }
    }
 }