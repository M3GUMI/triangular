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
#include "binanceorderwebsocketclient.h"

using namespace websocketclient;
#define access_key "c52zdrltx6vSMgojFzxJcVQ1v7qiD55G0PgTe31v3fCfEazqgnBu3xNRWOPVOj86"
#define secret_key "lDOZfpTNBIG8ICteeNfoOIoOHBONvBsiAP88GJ5rgDMF6bGGPETkM1Ri14mrbkfJ"

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
    const std::string hostname = "stream.binance.com";
    const std::string hostport = "9443";

    // 获取币安密钥
    std::string listenKey;
    void CreateAndSubListenKey();
    void CreateListkeyHandler(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec);
    void listenkeyKeepHandler();
    std::shared_ptr<websocketpp::lib::asio::steady_timer> listenkeyKeepTimer;
    websocketpp::lib::asio::io_service ioService;
    binanceorderwebsocketclient orderclient;

    void WSMsgHandler(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg);
    void ExecutionReportHandler(const rapidjson::Document &msg);

    void Subscribedepth(std::string fromToken,std::string toToken);
};