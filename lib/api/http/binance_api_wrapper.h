#pragma once
#include <string.h>
#include <functional>
#include "lib/libmix/libmix.h"
#include "lib/ahttp/http_client.hpp"
#include "define/define.h"
#include "base_api_wrapper.h"

using namespace std;

namespace HttpApi
{
    struct BinanceSymbolData {
        string Symbol;
        string BaseToken;
        string QuoteToken;
        double TicketSize;
    };

    class BinanceApiWrapper: public BaseApiWrapper
    {
    private:
        map<string, BinanceSymbolData> symbolMap;

        string sideToString(uint32_t side);
        string orderTypeToString(uint32_t orderType);
        string timeInForceToString(uint32_t timeInForce);

        void initBinanceSymbolCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec);
        void createOrderCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec);

        void cancelOrder(string orderId, string symbol);
        void cancelOrderCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec, std::string ori_symbol);

        void createListkeyCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec, function<void(int errCode, string listenKey)> callback);

    public:
        BinanceApiWrapper(websocketpp::lib::asio::io_service& ioService);
        ~BinanceApiWrapper();

        // 交易对
        void InitBinanceSymbol();
        BinanceSymbolData& GetSymbolData(std::string token0, std::string token1);
        BinanceSymbolData& GetSymbolData(std::string symbol);
        string GetSymbol(std::string token0, std::string token1);
        define::OrderSide GetSide(std::string token0, std::string token1);

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