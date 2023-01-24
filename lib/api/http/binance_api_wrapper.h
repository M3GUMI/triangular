#pragma once

#define ASIO_STANDALONE
#include <string.h>
#include <functional>
#include "openssl/hmac.h"
#include "lib/libmix/libmix.h"
#include "lib/ahttp/http_client.hpp"
#include "base_api_wrapper.h"

using namespace std;

namespace HttpApi
{
    class BinanceApiWrapper: public BaseApiWrapper
    {
    private:
        std::map<std::string, double> m_mapExchange2Ticksize;
        string sideToString(uint32_t side);
        string orderTypeToString(uint32_t orderType);
        string timeInForceToString(uint32_t timeInForce);

        void createOrderCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec);
        void cancelOrder(string orderId, string symbol);
        void cancelOrderCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec, std::string ori_symbol);

        void createListkeyCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec, function<void(int errCode, string listenKey)> callback);

    public:
        BinanceApiWrapper(websocketpp::lib::asio::io_service *ioService);
        ~BinanceApiWrapper();

        // 初始化交易对
        void binanceInitSymbol();
        void ExchangeInfoHandler(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec);

        // 创建订单
        int CreateOrder(CreateOrderReq &req);

        // 取消订单
        void CancelOrder(string orderId);
        void CancelOrderSymbol(vector<pair<string, string>> symbols);
        void CancelOrderAll();

        // listenKey
        void CreateListenKey(string listenKey, function<void(int errCode, string listenKey)> callback);
    };
}