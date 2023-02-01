#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include "openssl/hmac.h"
#include "lib/libmix/libmix.h"
#include "lib/ahttp/http_client.hpp"
#include "utils/utils.h"
#include "base_api_wrapper.h"

using namespace std;
namespace HttpWrapper
{
    BaseApiWrapper::BaseApiWrapper(websocketpp::lib::asio::io_service& ioService, string accessKey, string secretKey): ioService(ioService), accessKey(accessKey), secretKey(secretKey)
    {
        HttpClient *client = new HttpClient(ioService);
        hclientPtr = std::shared_ptr<HttpClient>(client);
        hclientPtr->set_timeout(5000);
    }

    BaseApiWrapper::~BaseApiWrapper()
    {
    }

    string BaseApiWrapper::GetOrderId(string outOrderId)
    {
        if (outOrderIdMap.count(outOrderId))
        {
            return outOrderIdMap[outOrderId];
        }

        return "";
    }

    string BaseApiWrapper::GetOutOrderId(string orderId)
    {
        if (orderIdMap.count(orderId))
        {
            return orderIdMap[orderId];
        }

        return "";
    }

    int BaseApiWrapper::CheckResp(shared_ptr<HttpRespone> &res)
    {
        if (res == nullptr || res->payload().empty())
        {
            LogError("func", "CheckResp", "msg", define::WrapErr(define::ErrorHttpFail));
            return define::ErrorHttpFail;
        }

        if (res->http_status() != 200)
        {
            LogError("func", "CheckResp", "msg", define::WrapErr(define::ErrorEmptyResponse));
            return define::ErrorEmptyResponse;
        }

        return 0;
    }

    CheckRespWithCodeResp& BaseApiWrapper::CheckRespWithCode(shared_ptr<HttpRespone> &res)
    {
        CheckRespWithCodeResp resp;
        if (res == nullptr || res->payload().empty())
        {
            LogError("func", "CheckResp", "msg", define::WrapErr(define::ErrorHttpFail));
            resp.Err = define::ErrorHttpFail;
            return resp;
        }

        rapidjson::Document json;
        json.Parse(res->payload().c_str());
        if (res->http_status() != 200)
        {
            LogError("func", "CheckResp", "msg", define::WrapErr(define::ErrorEmptyResponse));
            resp.Err = define::ErrorEmptyResponse;
            resp.Code = json.FindMember("code")->value.GetInt();
            resp.Msg = json.FindMember("msg")->value.GetString();
            return resp;
        }

        resp.Err = 0;
        return resp;
    }

    pair<double, double> BaseApiWrapper::SelectPriceQuantity(CreateOrderReq req, define::OrderSide side)
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

    void BaseApiWrapper::MakeRequest(ApiRequest& req, function<void(shared_ptr<HttpRespone> res, const ahttp::error_code &ec)> callback)
    {
        auto args = req.args;
        auto method = req.method;
        auto uri = req.uri;
        auto data = req.data;
        auto needSign = req.sign;

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

        // todo 日志整合
        LogDebug("func", "MakeRequest", "uri", whole_uri);
        LogDebug("method", method, "data", data);

        auto httpRequest = HttpRequest::make_request(method, whole_uri, data);
        httpRequest->Header = header;
        hclientPtr->Do(httpRequest, callback);

        LogDebug("func", "MakeRequest", "msg", "send request success");
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
                // todo 奇怪依赖问题
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