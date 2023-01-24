#pragma once
#include <functional>
#include <set>
#include <string.h>
#include "openssl/hmac.h"
#include "lib/libmix/libmix.h"
#include "lib/ahttp/http_client.hpp"
#include "defined/commondef.h"
#include "websocketpp/client.hpp"
#include "websocketpp/config/asio_client.hpp"
#include <bits/stdc++.h>

using namespace std;
namespace HttpApi
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
        commondef::OrderType OrderType;
        commondef::TimeInFoce TimeInForce;
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
        // 签名&发送请求
        string accessKey, secretKey;

        map<string, string> sign(map<string, string> &args, string &query, bool need_sign); // 需要根据特定重写
        string toURI(const map<string, string> &mp);                                        // 需要重写

        template <typename T, typename outer>
        string hmac(const std::string &key, const std::string &data, T evp, outer filter);
    public:
        BaseApiWrapper();
        BaseApiWrapper(websocketpp::lib::asio::io_service *ioService, string accessKey, string secretKey);
        ~BaseApiWrapper();

        // 获取symbol对

        // symobl储存
        std::map<std::string, std::string> baseOfSymbol;
        std::map<pair<std::string, std::string>, std::string> symbolMap;
        std::map<std::string, uint32_t> mapSymbolBalanceOrderStatus;

        uint32_t init_time = 0;
        std::shared_ptr<HttpClient> hclientPtr;
        websocketpp::lib::asio::io_service *ioService;

        std::set<std::string> m_setBaseCoins;                                        // 所有交易对的基础货币
        std::map<std::string, uint32_t> m_mapCoins2Index;                            // 货币名到下标
        std::map<uint32_t, std::string> m_mapIndex2Coins;                            // 下标到货币名
        std::map<std::string, std::pair<std::string, std::string>> m_mapSymbol2Base; // 交易对到货币名
        std::map<std::string, double> m_mapExchange2Ticksize;

        string GetClientOrderId(string orderId);

        void MakeRequest(ApiRequest& req);
        pair<std::string, commondef::OrderSide> getSymbolSide(std::string token0, std::string token1);
        pair<double, double> getPriceQuantity(CreateOrderReq req, commondef::OrderSide side);
    };
}