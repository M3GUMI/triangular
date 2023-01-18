#include "binancerestapiwrapper.h"
#include "binancecommondef.h"

using namespace binancecommondef;

namespace BinanceRestApiWrapper
{
    BinanceRestApiWrapper::BinanceRestApiWrapper(websocketpp::lib::asio::io_service &ioService, string accessKey, string secretKey) : m_ioService(&ioService), m_accessKey(accessKey), m_secretKey(secretKey)
    {
        HttpClient *client = new HttpClient(ioService);
        hclientPtr = std::shared_ptr<HttpClient>(client);
        hclientPtr->set_timeout(5000);
    }

    BinanceRestApiWrapper::~BinanceRestApiWrapper()
    {
    }

    void BinanceRestApiWrapper::BinanceMakeRequest(map<string, string> args, string method, string uri, string data, std::function<void(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec)> func, bool need_sign)
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

    void BinanceRestApiWrapper::CreateOrder(std::string symbol, uint32_t side, uint32_t order_type, uint32_t timeInForce, double quantity, double price, std::function<void(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec)> func)
    {
        std::string uri = "https://api.binance.com/api/v3/order";

        std::string strSide, strType, strTimeInForce;

        switch (side)
        {
        case Order_Side::BUY:
            strSide = "BUY";
            break;
        case Order_Side::SELL:
            strSide = "SELL";
            break;
        }

        switch (order_type)
        {
        case Order_Type::LIMIT:
            strType = "LIMIT";
            break;
        case Order_Type::MARKET:
            strType = "MARKET";
            break;
        case Order_Type::STOP_LOSS:
            strType = "STOP_LOSS";
            break;
        case Order_Type::STOP_LOSS_LIMIT:
            strType = "STOP_LOSS_LIMIT";
            break;
        case Order_Type::TAKE_PROFIT:
            strType = "TAKE_PROFIT";
            break;
        case Order_Type::TAKE_PROFIT_LIMIT:
            strType = "TAKE_PROFIT_LIMIT";
            break;
        case Order_Type::LIMIT_MAKER:
            strType = "LIMIT_MAKER";
            break;
        }

        switch (timeInForce)
        {
        case TimeInFoce::GTC:
            strTimeInForce = "GTC";
            break;
        case TimeInFoce::IOC:
            strTimeInForce = "IOC";
            break;
        case TimeInFoce::FOK:
            strTimeInForce = "FOK";
            break;
        }

        // char buf[256];
        // sprintf(buf,
        //         "{\"symbol\":%s,\"side\":%s,\"type\":%s,\"quantity\":%lf,\"price\":%lf,\"recvWindow\":\"5000\",\"timestamp\":%ull,\"timeInForce\":%s}",
        //         symbol.c_str(), strSide.c_str(), strType.c_str(), quantity, price, time(NULL) * 1000, strTimeInForce.c_str());

        map<string, string> args;
        args["symbol"] = symbol;
        args["timestamp"] = std::to_string(time(NULL) * 1000);
        args["side"] = strSide;
        args["type"] = strType;
        args["quantity"] = to_string(quantity);

        // 市价单 不能有下面这些参数
        if (order_type != Order_Type::MARKET && order_type != Order_Type::LIMIT_MAKER)
        {
            args["timeInForce"] = strTimeInForce;
            args["price"] = to_string(price);
        }

        if (order_type != Order_Type::MARKET)
        {
            args["price"] = to_string(price);
        }

        cout << "Create order" << " " << symbol << " " << side << " " << price << " " << quantity << endl;
    
        BinanceMakeRequest(args, "POST", uri, "", func, true);
    }

    std::map<std::string, std::string> BinanceRestApiWrapper::sign(std::map<std::string, std::string> &args, std::string &query, bool need_sign)
    {
        std::map<std::string, std::string> header;
        header["X-MBX-APIKEY"] = m_accessKey;

        if (not args.empty())
        {
            query = toURI(args);
            if (need_sign)
            {
                args["signature"] = hmac(m_secretKey, query, EVP_sha256(), Strings::hex_to_string);
                query = toURI(args);
            }
        }
        
        return header;
    }

    string BinanceRestApiWrapper::toURI(const map<string, string> &mp)
    {
        string str;
        for (auto it = mp.begin(); it != mp.end(); it++)
        {
            str += it->first + "=" + Strings::uri_escape_string(it->second) + "&";
        }
        return str.empty() ? str : str.erase(str.size() - 1);
    }

    template <typename T, typename outer>
    std::string BinanceRestApiWrapper::hmac(const std::string &key, const std::string &data, T evp, outer filter)
    {
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
}