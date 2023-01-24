#include <functional>
#include <string>
#include "openssl/hmac.h"
#include "lib/libmix/libmix.h"
#include "lib/ahttp/http_client.hpp"
#include "defined/defined.h"
#include "defined/commondef.h"
#include "base_api_wrapper.h"
#include "websocketpp/client.hpp"
#include "websocketpp/config/asio_client.hpp"
#include <rapidjson/document.h>

using namespace std;
namespace HttpApi
{
    BaseApiWrapper::BaseApiWrapper()
    {
    }

    BaseApiWrapper::BaseApiWrapper(websocketpp::lib::asio::io_service *ioService, string accessKey, string secretKey)
    {
        this->ioService = ioService;
        this->accessKey = accessKey;
        this->secretKey = secretKey;
    }

    BaseApiWrapper::~BaseApiWrapper()
    {
    }

    string GetClientOrderId(string orderId)
    {
        return "";
    }

    // 交易对
    pair<std::string, commondef::OrderSide> BaseApiWrapper::getSymbolSide(std::string token0, std::string token1)
    {
        if (symbolMap.count(make_pair(token0, token1)) == 1)
        {
            std::string symbol = symbolMap[make_pair(token0, token1)];
            std::string base = baseOfSymbol[symbol];
            commondef::OrderSide side;
            base == token0 ? side = commondef::SELL : side = commondef::BUY;
            return make_pair(symbol, side);
        }
        cout << "not found error" << endl;
        return make_pair("null", commondef::UNKNOWN);
    }

    pair<double, double> BaseApiWrapper::getPriceQuantity(CreateOrderReq req, commondef::OrderSide side)
    {
        double price, quantity;
        if (side == commondef::SELL)
        {
            price = req.FromPrice;
            quantity = req.FromQuantity;
        }
        if (side == commondef::BUY)
        {
            price = req.ToPrice;
            quantity = req.ToQuantity;
        }

        return make_pair(price, quantity);
    }

    // 签名
    void BaseApiWrapper::MakeRequest(ApiRequest& req)
    {
        // todo 奇怪的包问题，需要看一下
        /*auto args = req.args;
        auto method = req.method;
        auto uri = req.uri;
        auto data = req.data;
        auto needSign = req.sign;
        auto callback = req.callback;

        string query;
        const auto &header = sign(args, query, needSign);

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
        hclientPtr->Do(req, callback);

        cout << "End request" << endl;*/
    }

    std::map<std::string, std::string> BaseApiWrapper::sign(std::map<std::string, std::string> &args, std::string &query, bool need_sign)
    {
        std::map<std::string, std::string> header;
        header["X-MBX-APIKEY"] = accessKey;

        if (not args.empty())
        {
            query = toURI(args);
            if (need_sign)
            {
                // todo 临时测试
                // args["signature"] = this->hmac(secretKey, query, EVP_sha256(), Strings::hex_to_string);
                query = toURI(args);
            }
        }

        return header;
    }

    template <typename T, typename outer>
    std::string BaseApiWrapper::hmac(const std::string &key, const std::string &data, T evp, outer filter)
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

    std::string BaseApiWrapper::toURI(const map<std::string, std::string> &mp)
    {

        string str;
        for (auto it = mp.begin(); it != mp.end(); it++)
        {
            str += it->first + "=" + Strings::uri_escape_string(it->second) + "&";
        }
        return str.empty() ? str : str.erase(str.size() - 1);
    }
}