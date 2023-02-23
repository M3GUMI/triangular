#pragma once
#include <string.h>
#include <functional>
#include "lib/libmix/libmix.h"
#include "lib/ahttp/http_client.hpp"
#include "define/define.h"
#include "define/error.h"
#include "base_api_wrapper.h"

using namespace std;
namespace HttpWrapper
{
    struct BinanceSymbolData {
        string Symbol;
        string BaseToken;
        string QuoteToken;
        double TicketSize = 0;
        double MinNotional = 0;
    };

    struct BalanceData
    {
        string Token;
        double Free = 0;
        double Locked = 0;
    };

    struct AccountInfo
    {
        vector<BalanceData> Balances;
    };

    class BinanceApiWrapper: public BaseApiWrapper
    {
    private:
        map<string, BinanceSymbolData> symbolMap;
        vector<function<void(map<string, BinanceSymbolData> &data)>> symbolReadySubscriber;

        define::OrderStatus stringToOrderStatus(string status);
        define::OrderSide stringToSide(string side);
        string sideToString(uint32_t side);
        string orderTypeToString(uint32_t orderType);
        string timeInForceToString(uint32_t timeInForce);
        pair<string, string> parseToken(string symbol, define::OrderSide side);

        void initBinanceSymbolCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec);
        void accountInfoHandler(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec, function<void(AccountInfo &info, int err)> callback);
        void createOrderCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec, OrderData &req, function<void(OrderData& data, int err)> callback);

        void cancelOrder(uint64_t orderId, string symbol);
        void cancelOrderCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec, std::string ori_symbol);

        void createListkeyCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec, function<void(string listenKey, int err)> callback);

    public:
        BinanceApiWrapper(websocketpp::lib::asio::io_service &ioService);
        ~BinanceApiWrapper();

        // 交易对
        void InitBinanceSymbol();
        void SubscribeSymbolReady(function<void(map<string, BinanceSymbolData> &data)> callback);
        BinanceSymbolData &GetSymbolData(std::string token0, std::string token1);
        BinanceSymbolData &GetSymbolData(std::string symbol);
        string GetSymbol(std::string token0, std::string token1);
        define::OrderSide GetSide(std::string token0, std::string token1);

        // 账户信息
        int GetAccountInfo(function<void(AccountInfo &info, int err)> callback);

        // 创建订单
        int CreateOrder(OrderData& req, function<void(OrderData& data, int err)> callback);

        // 取消订单
        void CancelOrder(uint64_t orderId);
        void CancelOrderSymbol(vector<pair<string, string>> symbols);
        void CancelOrderAll();

        // listenKey
        void CreateListenKey(string listenKey, function<void(string listenKey, int err)> callback);
    };
}