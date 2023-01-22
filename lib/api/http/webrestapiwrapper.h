#pragma once

#define ASIO_STANDALONE
#include <bits/stdc++.h>
#include <functional>

#include "../ws/websocketclient.h"

#include "openssl/hmac.h"
#include "../../libmix/libmix.h"
#include "../../ahttp/http_client.hpp"

using namespace std;

namespace webrestapiwrapper
{
    class webrestapiwrapper
    {
    public:
        webrestapiwrapper(websocketpp::lib::asio::io_service &ioService, std::string accessKey, std::string secretKey); // 公钥 密钥配置构造web_client
        webrestapiwrapper();
        ~webrestapiwrapper();

        void MakeRequest(map<std::string, std::string> args, std::string method, std::string uri, string data, std::function<void(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec)> func, bool need_sign);
        void CreateOrder(std::string symbol, uint32_t side, uint32_t order_type, uint32_t timeInForce, double quantity, double price, std::function<void(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec)> func, std::string uri);
        //签名
        std::map<std::string, std::string> sign(std::map<std::string, std::string> &args, std::string &query, bool need_sign); // 需要根据特定重写
        std::string toURI(const map<std::string, std::string> &mp); //需要重写
        
        // 密钥储存
        std::string accessKey, secretKey;
        std::shared_ptr<HttpClient> hclientPtr;
        websocketpp::lib::asio::io_service *ioService;

         static void String2Double(const string &str, double &d);
         static uint64_t getTime() {};
    };
};
