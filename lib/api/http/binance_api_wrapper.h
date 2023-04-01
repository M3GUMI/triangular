#pragma once
#include <string.h>
#include <functional>
#include "lib/order/order.h"
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
        double StepSize = 0;
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
        vector<function<void(vector<BinanceSymbolData> &data)>> symbolReadySubscriber;

        void initBinanceSymbolCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec);
        void accountInfoHandler(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec, function<void(AccountInfo &info, int err)> callback);
        void getOpenOrderCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec);
        void getUserAssetHandler(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec, function<void(double btc)> callback);
        void createOrderCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec, OrderData &req, function<void(OrderData& data, int err)> callback);

        void cancelOrder(uint64_t orderId, string symbol);
        void cancelOrderCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec, std::string ori_symbol);
        void cancelOrderId(uint64_t orderId, function<void(int err, int orderId)> callback);
        void cancelOrderIdCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec, int orderId, function<void(int orderId, int err)> callback);

        void createListkeyCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec, function<void(string listenKey, int err)> callback);
        void keepListenKeyCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec);

    public:
        BinanceApiWrapper(websocketpp::lib::asio::io_service &ioService);
        ~BinanceApiWrapper();

        // 交易对
        void InitBinanceSymbol();
        void SubscribeSymbolReady(function<void(vector<BinanceSymbolData> &data)> callback);
        BinanceSymbolData &GetSymbolData(const std::string &token0, const std::string& token1);
        BinanceSymbolData &GetSymbolData(const std::string& symbol);
        string GetSymbol(const std::string& token0, const std::string& token1);
        define::OrderSide GetSide(const std::string &token0, const std::string& token1);

        // 账户信息
        int GetAccountInfo(function<void(AccountInfo &info, int err)> callback);
        int GetOpenOrder(string symbol);
        int GetUserAsset(function<void(double btc)> callback);

        // 创建订单
        int CreateOrder(OrderData& req, function<void(OrderData& data, int err)> callback);

        // 取消订单
        void CancelOrder(uint64_t orderId, function<void(int orderId, int err)> callback);
        void CancelOrderSymbol(string token0, string token1);
        void CancelOrderSymbols(vector<pair<string, string>> symbols);
        void CancelOrderAll();

        // listenKey
        void CreateListenKey(function<void(string listenKey, int err)> callback);
        void KeepListenKey(string listenKey);
    };
}