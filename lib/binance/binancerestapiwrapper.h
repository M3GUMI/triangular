#pragma once

#define ASIO_STANDALONE
#include <bits/stdc++.h>
#include <functional>

#include "binancewebsocketwrapper.h"

#include "openssl/hmac.h"
#include "lib/libmix/libmix.h"
#include "lib/ahttp/http_client.hpp"

using namespace std;

namespace BinanceRestApiWrapper
{

    class BinanceRestApiWrapper
    {
    public:
        BinanceRestApiWrapper(websocketpp::lib::asio::io_service &ioService, string accessKey, string secretKey);
        ~BinanceRestApiWrapper();

        void BinanceMakeRequest(map<string, string> args, string method, string uri, string data, std::function<void(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec)> func, bool need_sign);
        void CreateOrder(std::string symbol, uint32_t side, uint32_t order_type, uint32_t timeInForce, double quantity, double price, std::function<void(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec)> func);

    private:
        string m_accessKey, m_secretKey;
        std::shared_ptr<HttpClient> hclientPtr;
        websocketpp::lib::asio::io_service *m_ioService;

        std::map<std::string, std::string> sign(std::map<std::string, std::string> &args, std::string &query, bool need_sign);
        string toURI(const map<string, string> &mp);
        template <typename T, typename outer>
        std::string hmac(const std::string &key, const std::string &data, T evp, outer filter);
    };
}