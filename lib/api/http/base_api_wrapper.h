#pragma once
#include <set>
#include <string.h>
#include <functional>
#include "websocketpp/config/asio_client.hpp"
#include "define/define.h"

using namespace std;
namespace HttpWrapper
{
    struct ApiRequest
    {
        map<string, string> args;
        string method;
        string uri;
        string data;
        bool sign;

        function<void(shared_ptr<HttpRespone> res, const ahttp::error_code &ec)> callback;

        ApiRequest()
        {
            this->args = {};
            this->method = "";
            this->uri = "";
            this->data = "";
            this->sign = false;
            this->callback = NULL;
        }
    };

    struct CreateOrderReq
    {
        string FromToken;
        double FromPrice;
        double FromQuantity;
        string ToToken;
        double ToPrice;
        double ToQuantity;
        define::OrderType OrderType;
        define::TimeInForce TimeInForce;
    };

    struct CancelOrderReq
    {
        uint64_t OrderId;
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

        // 签名
        string accessKey, secretKey;
        map<string, string> sign(map<string, string> &args, string &query, bool need_sign);
        string toURI(const map<string, string> &mp);
        template <typename T, typename outer>
        string hmac(const std::string &key, const std::string &data, T evp, outer filter);

    public:
        BaseApiWrapper(websocketpp::lib::asio::io_service& ioService, string accessKey, string secretKey);
        ~BaseApiWrapper();

        // 交易对基础货币
        set<string> baseCoins;

        string GetClientOrderId(string orderId); // todo 待补充
        pair<double, double> GetPriceQuantity(CreateOrderReq req, define::OrderSide side);
        void MakeRequest(ApiRequest& req);
    };
}