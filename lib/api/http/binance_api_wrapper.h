#pragma once
#include <string.h>
#include <functional>
#include "lib/libmix/libmix.h"
#include "lib/ahttp/http_client.hpp"
#include "define/define.h"
#include "base_api_wrapper.h"

using namespace std;
namespace HttpWrapper
{
    struct BinanceSymbolData {
        string Symbol;
        string BaseToken;
        string QuoteToken;
        double TicketSize;
    };

    struct BalanceData
    {
        string Asset;
        double Free;
        double Locked;
    };

    struct AccountInfo
    {
        vector<BalanceData> Balances;
    };

	struct OrderData
	{
        string OrderId;
        uint64_t ExecuteTime;
        define::OrderStatus OrderStatus;
        std::string FromToken;
        std::string ToToken;
        double ExecutePrice;
        double ExecuteQuantity;
        double OriginQuantity;
    };

    class BinanceApiWrapper: public BaseApiWrapper
    {
    private:
        map<string, BinanceSymbolData> symbolMap;

        define::OrderStatus stringToOrderStatus(string status);
        define::OrderSide stringToSide(string side);
        string sideToString(uint32_t side);
        string orderTypeToString(uint32_t orderType);
        string timeInForceToString(uint32_t timeInForce);
        pair<string, string> parseToken(string symbol, define::OrderSide side);

        void initBinanceSymbolCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec);
        void accountInfoHandler(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec, function<void(AccountInfo &info)> callback);
        void createOrderCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec, string orderId, function<void(OrderData& data)> callback);

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

        // 账户余额
        int GetAccountInfo(function<void(AccountInfo &info)> callback);

        // 创建订单
        int CreateOrder(CreateOrderReq &req, function<void(OrderData& data)> callback);

        // 取消订单
        void CancelOrder(string orderId);
        void CancelOrderSymbol(vector<pair<string, string>> symbols);
        void CancelOrderAll();

        // listenKey
        void CreateListenKey(string listenKey, function<void(int errCode, string listenKey)> callback);
    };
}