#pragma once
#define ASIO_STANDALONE
#include <bits/stdc++.h>
#include <functional>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/reader.h>
#include <rapidjson/stringbuffer.h>
#include "websocketpp/client.hpp"
#include "boost/asio/ssl/context_base.hpp"
#include "websocketpp/config/asio_client.hpp"
#include "depthwebsocketclient.h"
#include "../../ahttp/http_client.hpp" //配置include另外文件夹即可解决
#include "../http/binancewebrestapiwrapper.h"

using namespace websocketclient;
#define access_key "ppp"
#define secret_key "ddd"

class binancedepthwebsocketclient : public depthwebsocketclient
{
public:
    binancedepthwebsocketclient(std::string conectionName, websocketpp::lib::asio::io_service *ioService, string msg);
    binancedepthwebsocketclient();
    ~binancedepthwebsocketclient();

    // 币安信息发送器
    binancewebrestapiwrapper *restapi_client;

    // 币安ws连接器
    binancedepthwebsocketclient *ws_client;

    // 获取币安密钥
    std::string listenKey;
    void CreateAndSubListenKey();
    void CreateListkeyHandler(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec);
    void listenkeyKeepHandler();
    std::shared_ptr<websocketpp::lib::asio::steady_timer> listenkeyKeepTimer;
    websocketpp::lib::asio::io_service ioService;
};