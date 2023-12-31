#include "client.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include "openssl/hmac.h"
#include "lib/libmix/libmix.h"
#include "lib/ahttp/http_client.hpp"
#include "conf/conf.h"
#include "utils/utils.h"
#include "base_api_wrapper.h"

using namespace std;
namespace HttpWrapper
{
    BaseApiWrapper::BaseApiWrapper(websocketpp::lib::asio::io_service& ioService, string accessKey, string secretKey): ioService(ioService), accessKey(accessKey), secretKey(secretKey)
    {
        HttpClient *client = new HttpClient(ioService);
        hclientPtr = std::shared_ptr<HttpClient>(client);
        hclientPtr->set_timeout(10000);
    }

    BaseApiWrapper::~BaseApiWrapper()
    {
    }

    uint64_t BaseApiWrapper::GetOrderId(const string& outOrderId)
    {
        if (outOrderIdMap.count(outOrderId))
        {
            return outOrderIdMap[outOrderId];
        }

        return 0;
    }

    string BaseApiWrapper::GetOutOrderId(uint64_t orderId)
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
            spdlog::error("func: {}, err: {}", "CheckResp", WrapErr(define::ErrorHttpFail));
            return define::ErrorEmptyResponse;
        }

        if (res->http_status() != 200)
        {
            spdlog::error("func: {}, err: {}, resp: {]", "CheckResp", WrapErr(define::ErrorEmptyResponse), res->payload());
            return define::ErrorHttpFail;
        }

        return 0;
    }

    CheckRespWithCodeResp BaseApiWrapper::CheckRespWithCode(shared_ptr<HttpRespone> &res)
    {
        CheckRespWithCodeResp resp;
        if (res == nullptr || res->payload().empty())
        {
            spdlog::error("func: CheckRespWithCode, err: {}, resp: {}", WrapErr(define::ErrorEmptyResponse), res->payload());
            resp.Err = define::ErrorEmptyResponse;
            return resp;
        }

        rapidjson::Document json;
        json.Parse(res->payload().c_str());
        if (res->http_status() != 200)
        {

            spdlog::error("func: CheckRespWithCode, err: {}, resp: {}", WrapErr(define::ErrorHttpFail), res->payload());
            resp.Err = define::ErrorHttpFail;
            resp.Code = json.FindMember("code")->value.GetInt();
            resp.Msg = json.FindMember("msg")->value.GetString();
            return resp;
        }

        resp.Err = 0;
        return resp;
    }

    void BaseApiWrapper::MakeRequest(ApiRequest& req, function<void(shared_ptr<HttpRespone> res, const ahttp::error_code &ec)> callback)
    {
        if (conf::EnableMock)
        {
            mockTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService,
                                                                                  websocketpp::lib::asio::milliseconds(10));
            mockTimer->async_wait(bind(&BaseApiWrapper::mockCallback, this, callback));
            return;
        }

        return makeRequest(req, callback);
    }

    void BaseApiWrapper::MakeRequest(ApiRequest &req, function<void(shared_ptr<HttpRespone> res, const ahttp::error_code &ec)> callback, bool noMock)
    {
        if (noMock) {
            return makeRequest(req, callback);
        }

        return MakeRequest(req, callback);
    }

    void BaseApiWrapper::makeRequest(ApiRequest& req, function<void(shared_ptr<HttpRespone> res, const ahttp::error_code &ec)> callback)
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

        auto httpRequest = HttpRequest::make_request(method, whole_uri, data);
        httpRequest->Header = header;
        hclientPtr->Do(httpRequest, callback);
        spdlog::debug("func: MakeRequest, method: {}, data: {}, url: {}", method, data, whole_uri);
    }

    void BaseApiWrapper::mockCallback(function<void(shared_ptr<HttpRespone> res, const ahttp::error_code &ec)> callback)
    {
        shared_ptr<HttpRespone> res;
        ahttp::error_code ec;
        return callback(res, ec);
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