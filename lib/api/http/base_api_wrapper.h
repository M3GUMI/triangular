#pragma once
#include <set>
#include <string.h>
#include <functional>
#include "websocketpp/config/asio_client.hpp"
#include "define/define.h"
#include "utils/utils.h"

using namespace std;
namespace HttpWrapper
{
    struct CheckRespWithCodeResp
    {
        int Err = 0;
        int Code = 0;
        string Msg;
    };

    struct ApiRequest
    {
        map<string, string> args;
        string method;
        string uri;
        string data;
        bool sign = false;
    };

    struct CancelOrderReq
    {
        uint64_t OrderId = 0;
    };

    struct CancelOrderSymbolReq
    {
        vector<pair<string, string>> pairs;
    };

    class BaseApiWrapper
    {
    private:
        std::shared_ptr<HttpClient> hclientPtr;
        websocketpp::lib::asio::io_service &ioService;
        std::shared_ptr<websocketpp::lib::asio::steady_timer> mockTimer; // mock计时器

        // 签名
        string accessKey, secretKey;
        void makeRequest(ApiRequest &req, function<void(shared_ptr<HttpRespone> res, const ahttp::error_code &ec)> callback);
        void mockCallback(function<void(shared_ptr<HttpRespone> res, const ahttp::error_code &ec)> callback);
        map<string, string> sign(map<string, string> &args, string &query, bool need_sign);
        string toURI(const map<string, string> &mp);
        template <typename T, typename outer>
        string hmac(const std::string &key, const std::string &data, T evp, outer filter);

    public:
        BaseApiWrapper(websocketpp::lib::asio::io_service& ioService, string accessKey, string secretKey);
        ~BaseApiWrapper();

        // 交易对基础货币
        set<string> baseCoins;
        // 订单号映射
        map<uint64_t, string> orderIdMap; // 内部id转外部id
        map<string, uint64_t> outOrderIdMap; // 外部id转内部id

        uint64_t GetOrderId(const string& outOrderId);
        string GetOutOrderId(uint64_t orderId);

        int CheckResp(shared_ptr<HttpRespone> &res);
        CheckRespWithCodeResp CheckRespWithCode(shared_ptr<HttpRespone> &res);
        void MakeRequest(ApiRequest &req, function<void(shared_ptr<HttpRespone> res, const ahttp::error_code &ec)> callback);
        void MakeRequest(ApiRequest &req, function<void(shared_ptr<HttpRespone> res, const ahttp::error_code &ec)> callback, bool mock);
    };
}