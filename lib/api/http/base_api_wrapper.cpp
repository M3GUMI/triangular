#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include "openssl/hmac.h"
#include "lib/libmix/libmix.h"
#include "lib/ahttp/http_client.hpp"
#include "base_api_wrapper.h"

using namespace std;
namespace HttpWrapper
{
    BaseApiWrapper::BaseApiWrapper(websocketpp::lib::asio::io_service& ioService, string accessKey, string secretKey): ioService(ioService), accessKey(accessKey), secretKey(secretKey)
    {
    }

    BaseApiWrapper::~BaseApiWrapper()
    {
    }

    string GetClientOrderId(string orderId)
    {
        return "";
    }

    pair<double, double> BaseApiWrapper::GetPriceQuantity(CreateOrderReq req, define::OrderSide side)
    {
        double price, quantity;
        if (side == define::SELL)
        {
            price = req.FromPrice;
            quantity = req.FromQuantity;
        }
        if (side == define::BUY)
        {
            price = req.ToPrice;
            quantity = req.ToQuantity;
        }

        return make_pair(price, quantity);
    }

    void BaseApiWrapper::MakeRequest(ApiRequest& req)
    {
        auto args = req.args;
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

        auto httpRequest = HttpRequest::make_request(method, whole_uri, data);
        httpRequest->Header = header;
        hclientPtr->Do(httpRequest, callback);

        cout << "End request" << endl;
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
                args["signature"] = this->hmac(secretKey, query, EVP_sha256(), Strings::hex_to_string);
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

        // openssl 3.0.2
        HMAC(evp, key.data(), int(key.size()),
             (unsigned char *)data.data(), data.size(),
             mac, &mac_length);

        // openssl 1.0.2k
        // HMAC_CTX ctx;
        // HMAC_CTX_new(&ctx);
        // HMAC_Init_ex(&ctx, key.data(), int(key.size()), evp, NULL);
        // HMAC_Update(&ctx, (unsigned char *)data.data(), data.size());
        // HMAC_Final(&ctx, mac, &mac_length);
        // HMAC_CTX_reset(&ctx);

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