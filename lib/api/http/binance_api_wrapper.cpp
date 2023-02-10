#include <rapidjson/document.h>
#include "conf/conf.h"
#include "define/define.h"
#include "utils/utils.h"
#include "binance_api_wrapper.h"

using namespace std;
namespace HttpWrapper
{
    BinanceApiWrapper::BinanceApiWrapper(websocketpp::lib::asio::io_service &ioService) : BaseApiWrapper(ioService, conf::AccessKey, conf::SecretKey)
    {
    }

    BinanceApiWrapper::~BinanceApiWrapper()
    {
    }

    void BinanceApiWrapper::InitBinanceSymbol()
    {
        ApiRequest req;
        req.args = {};
        req.method = "GET";
        req.uri = "https://api.binance.com/api/v3/exchangeInfo";
        req.data = "";
        req.sign = false;
        MakeRequest(req, bind(&BinanceApiWrapper::initBinanceSymbolCallback, this, placeholders::_1, placeholders::_2), true);
    }

    void BinanceApiWrapper::SubscribeSymbolReady(function<void(map<string, BinanceSymbolData> &data)> callback)
    {
        this->symbolReadySubscriber.push_back(callback);
    }

    void BinanceApiWrapper::initBinanceSymbolCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec)
    {
        if (auto err = CheckResp(res); err > 0) {
            return;
        }

        rapidjson::Document exchangeInfoJson;
        exchangeInfoJson.Parse(res->payload().c_str());
        if (!exchangeInfoJson.HasMember("symbols"))
        {
            LogError("func", "initBinanceSymbolCallback", "msg", WrapErr(define::ErrorInvalidResp));
            return;
        }

        const auto &symbols = exchangeInfoJson["symbols"];
        for (unsigned i = 0; i < symbols.Size(); i++)
        {
            double doubleTickSize = 0;
            string symbol = symbols[i].FindMember("symbol")->value.GetString();
            string baseAsset = symbols[i].FindMember("baseAsset")->value.GetString();
            string quoteAsset = symbols[i].FindMember("quoteAsset")->value.GetString();
            if (baseAsset == "NGN" || quoteAsset == "NGN")
            {
                continue;
            }

            if (symbols[i].HasMember("filters"))
            {
                auto filters = symbols[i].FindMember("filters")->value.GetArray();
                for (uint32_t j = 0; j < filters.Size(); j++)
                {
                    if (not filters[j].HasMember("stepSize") || not filters[j].HasMember("filterType"))
                    {
                        continue;
                    }

                    string filterType = filters[j].FindMember("filterType")->value.GetString();

                    if (filterType != "LOT_SIZE")
                    {
                        continue;
                    }

                    string ticksize = filters[j].FindMember("stepSize")->value.GetString();
                    String2Double(ticksize, doubleTickSize);
                    break;
                }
            }

            BinanceSymbolData data;
            data.Symbol = symbol;
            data.BaseToken = baseAsset;
            data.QuoteToken = quoteAsset;
            data.TicketSize = doubleTickSize;

            symbolMap[baseAsset + quoteAsset] = data;
            symbolMap[quoteAsset + baseAsset] = data;
            baseCoins.insert(baseAsset);
            baseCoins.insert(quoteAsset);
        }

        LogInfo("func", "InitBinanceSymbol", "msg", "load symbol data success");
        for (auto func : this->symbolReadySubscriber)
        {
            func(symbolMap);
        }
    }

    BinanceSymbolData& BinanceApiWrapper::GetSymbolData(std::string token0, std::string token1)
    {
        auto symbol = toUpper(token0 + token1);
        if (symbolMap.count(symbol) != 1)
        {
            LogError("func", "GetSymbolData", "msg", "not found symbol", "err", WrapErr(define::ErrorDefault));
            BinanceSymbolData data;
            return data;
        }

        return symbolMap[symbol];
    }

    BinanceSymbolData& BinanceApiWrapper::GetSymbolData(std::string symbol)
    {
        if (symbolMap.count(toUpper(symbol)) != 1)
        {
            LogError("func", "GetSymbolData", "msg", "not found symbol", "err", WrapErr(define::ErrorDefault));
            BinanceSymbolData data;
            return data;
        }

        return symbolMap[symbol];
    }

    string BinanceApiWrapper::GetSymbol(std::string token0, std::string token1)
    {
        auto symbol = toUpper(token0 + token1);
        if (symbolMap.count(symbol) != 1)
        {
            LogError("func", "GetSymbol", "msg", "not found symbol", "err", WrapErr(define::ErrorDefault));
            return "";
        }

        BinanceSymbolData data = symbolMap[symbol];
        return data.Symbol;
    }

    define::OrderSide BinanceApiWrapper::GetSide(std::string token0, std::string token1)
    {
        auto symbol = toUpper(token0 + token1);
        if (symbolMap.count(symbol) != 1)
        {
            LogError("func", "GetSide", "msg", "not found symbol", "err", WrapErr(define::ErrorDefault));
            return define::INVALID_SIDE;
        }

        define::OrderSide side;
        BinanceSymbolData data = symbolMap[symbol];
        data.BaseToken == token0 ? side = define::SELL : side = define::BUY;
        return side;
    }

    int BinanceApiWrapper::GetAccountInfo(function<void(AccountInfo& info, int err)> callback)
    {
        uint64_t now = time(NULL);

        ApiRequest req;
        req.args["timestamp"] = to_string(now * 1000);
        req.method = "GET";
        req.uri = "https://api.binance.com/api/v3/account";
        req.data = "";
        req.sign = true;
        auto apiCallback = bind(&BinanceApiWrapper::accountInfoHandler, this, ::_1, ::_2, callback);

        this->MakeRequest(req, apiCallback);
        return 0;
    }

    void BinanceApiWrapper::accountInfoHandler(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec, function<void(AccountInfo& info, int err)> callback)
    {
        AccountInfo info;
        if (conf::EnableMock) {
            BalanceData data;
            data.Token = "OP";
            data.Free = 2000;
            data.Locked = 0;
            info.Balances.push_back(data);
            LogDebug("func", "GetAccountInfo", "msg", "mock account_info");
            return callback(info, 0);
        }

        if (auto err = this->CheckResp(res); err > 0)
        {
            return callback(info, err);
        }

        rapidjson::Document accountInfoDocument;
        accountInfoDocument.Parse(res->payload().c_str());
        if (not accountInfoDocument.HasMember("balances"))
        {
            LogError("func", "accountInfoHandler", "msg", "no balance data", "err", WrapErr(define::ErrorInvalidResp));
            return callback(info, define::ErrorDefault);
        }

        auto balances = accountInfoDocument["balances"].GetArray();
        for (int i = 0; i < balances.Size(); i++)
        {
            std::string asset = balances[i].FindMember("asset")->value.GetString();
            std::string free = balances[i].FindMember("free")->value.GetString();
            std::string locked = balances[i].FindMember("locked")->value.GetString();

            BalanceData data;
            data.Token = asset;
            String2Double(free, data.Free);
            String2Double(locked, data.Locked);
            info.Balances.push_back(data);
        }

        LogInfo("func", "GetAccountInfo", "msg", "get account_info success");
        return callback(info, 0);
    }

    int BinanceApiWrapper::CreateOrder(CreateOrderReq &req, function<void(OrderData& data, int err)> callback)
    {
        if (req.OrderId == "")
        {
            LogError("func", "CreateOrder", "msg", "orderId invalid", "err", WrapErr(define::ErrorInvalidParam));
            return define::ErrorInvalidParam;
        }

        map<string, string> args;
        string uri = "https://api.binance.com/api/v3/order";
        pair<std::string, define::OrderSide> symbolSide;
        auto symbol = GetSymbol(req.FromToken, req.ToToken);
        auto side = GetSide(req.FromToken, req.ToToken);

        if (symbol == "" || side == define::INVALID_SIDE)
        {
            LogError("func", "CreateOrder", "msg", "symbol or side invalid", "err", WrapErr(define::ErrorInvalidParam));
            return define::ErrorDefault;
        }

        pair<double, double> priceQuantity;
        priceQuantity = SelectPriceQuantity(req, side);
        auto price = priceQuantity.first;
        auto quantity = priceQuantity.second;

        args["symbol"] = symbol;
        args["side"] = this->sideToString(side);
        args["type"] = this->orderTypeToString(req.OrderType);
        args["timestamp"] = std::to_string(time(NULL) * 1000);
        args["quantity"] = quantity;

        // 市价单 不能有下面这些参数
        if (req.OrderType != define::MARKET && req.OrderType != define::LIMIT_MAKER)
        {
            args["timeInForce"] = this->timeInForceToString(req.TimeInForce);
        }

        if (req.OrderType != define::MARKET)
        {
            args["price"] = price;
        }

        LogDebug("func", "CreateOrder", "msg", "start execute", "symbol", symbol);
        LogDebug("side", to_string(side), "price", to_string(price), "quantity", to_string(quantity));

        ApiRequest apiReq;
        apiReq.args = args;
        apiReq.method = "POST";
        apiReq.uri = uri;
        apiReq.data = "";
        apiReq.sign = true;
        auto apiCallback = bind(&BinanceApiWrapper::createOrderCallback, this, placeholders::_1, placeholders::_2, req, callback);
        this->MakeRequest(apiReq, apiCallback);
        return 0;
    }

    void BinanceApiWrapper::createOrderCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec, CreateOrderReq &req, function<void(OrderData& data, int err)> callback)
    {
        OrderData data;
        if (conf::EnableMock) {
            data.OrderId = req.OrderId;
            data.UpdateTime = GetNowTime();
            data.OrderStatus = define::FILLED;
            data.FromToken = req.FromToken;
            data.ToToken = req.ToToken;
            data.ExecutePrice = req.FromPrice;
            data.ExecuteQuantity = req.FromQuantity;
            data.OriginQuantity = req.FromQuantity;

            // 最大成交500
            if (req.FromQuantity > 500) {
                data.ExecuteQuantity = req.FromQuantity - 500;
                data.OrderStatus = define::PARTIALLY_FILLED;
            }

            LogDebug("func", "createOrderCallback", "msg", "mock data");
            return callback(data, 0);
        }

        if (auto checkResult = this->CheckRespWithCode(res); checkResult.Err > 0)
        {
            if (checkResult.Code == -2010 && checkResult.Msg == "Account has insufficient balance for requested action.") {
                LogError("func", "CreateOrder", "err", WrapErr(define::ErrorInsufficientBalance));
                return callback(data, define::ErrorInsufficientBalance);
            }

            return callback(data, checkResult.Err);
        }

        rapidjson::Document order;
        order.Parse(res->payload().c_str());

        string clientOrderId = order.FindMember("clientOrderId")->value.GetString();
        orderIdMap[req.OrderId] = clientOrderId;
        outOrderIdMap[clientOrderId] = req.OrderId;

        // 当前均为IOC数据推送
        auto symbol = order["s"].GetString();
        auto side = stringToSide(order["S"].GetString());

        if (side == define::INVALID_SIDE)
        {
            LogError("func", "CreateOrder", "msg", "invalid side", "err", WrapErr(define::ErrorInvalidResp));
            return;
        }

        data.OrderId = this->orderIdMap[order["c"].GetString()];
        data.UpdateTime = order["E"].GetUint64();
        data.OrderStatus = stringToOrderStatus(order["X"].GetString());
        data.FromToken = parseToken(symbol, side).first; 
        data.ToToken = parseToken(symbol, side).second; 
        data.ExecutePrice = order["L"].GetDouble();
        data.ExecuteQuantity = order["l"].GetDouble();
        data.OriginQuantity = order["q"].GetDouble();

        return callback(data, 0);
    }

    void BinanceApiWrapper::cancelOrder(string orderId, string symbol)
    {
        map<string, string> args;
        string uri = "https://api.binance.com/api/v3/openOrders";
        args["symbol"] = symbol;
        args["timestamp"] = std::to_string(time(NULL) * 1000);
        args["recvWindow"] = "50000";
        args["newClientOrderId"] = GetOutOrderId(orderId);

        ApiRequest req;
        req.args = args;
        req.method = "DELETE";
        req.uri = uri;
        req.data = "";
        req.sign = true;
        auto callback = bind(&BinanceApiWrapper::cancelOrderCallback, this, placeholders::_1, placeholders::_2, symbol);

        MakeRequest(req, callback);
    }

    void BinanceApiWrapper::CancelOrder(string orderId)
    {
        this->cancelOrder(orderId, "");
    }

    void BinanceApiWrapper::CancelOrderSymbol(vector<pair<string, string>> symbols)
    {
        for (auto symbol : symbols)
        {
            auto symbolStr = this->GetSymbol(symbol.first, symbol.second);
            this->cancelOrder("", symbolStr);
        }
    }

    void BinanceApiWrapper::CancelOrderAll()
    {
        // todo 增加账号symbol扫描
        vector<pair<string, string>> symbols;
        this->CancelOrderSymbol(symbols);
    }

    void BinanceApiWrapper::cancelOrderCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec, std::string ori_symbol)
    {
        if (auto checkResult = this->CheckRespWithCode(res); checkResult.Err > 0)
        {
            if (checkResult.Code == -2011 && checkResult.Msg == "Unknown order sent.") {
                // todo error日志
                // 找不到订单
                // todo 改成回调通知订单管理层
                // mapSymbolBalanceOrderStatus[ori_symbol] = 0;
                return;
            }

            return;
        }

        // todo 改成回调通知订单管理层
        // mapSymbolBalanceOrderStatus[ori_symbol] = 0;
        return;
    }

    void BinanceApiWrapper::CreateListenKey(string listenKey, function<void(string listenKey, int err)> callback)
    {
        string uri = "https://api.binance.com/api/v3/userDataStream";
        ApiRequest req;
        req.args = {};
        req.method = "POST";
        req.uri = uri;
        req.data = "";
        req.sign = true;
        auto apiCallback = bind(&BinanceApiWrapper::createListkeyCallback, this, placeholders::_1, placeholders::_2, callback);

        if (listenKey.size() == 0)
        {
            req.args["listenKey"] = listenKey;
            req.method = "PUT";
        }

        this->MakeRequest(req, apiCallback);
    }

    void BinanceApiWrapper::createListkeyCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec, function<void(string listenKey, int err)> callback)
    {
        if (auto err = this->CheckResp(res); err > 0)
        {
            // todo error日志
            cout << __func__ << " " << __LINE__ << " CreateListkey error " << endl;
            return callback("", err);
        }

        rapidjson::Document listenKeyInfo;
        listenKeyInfo.Parse(res->payload().c_str());

        if (!listenKeyInfo.HasMember("listenKey"))
        {
            // todo error日志
            return callback("", define::ErrorDefault);
        }

        auto listenKey = listenKeyInfo.FindMember("listenKey")->value.GetString();
        return callback(listenKey, 0);
    }

    define::OrderStatus BinanceApiWrapper::stringToOrderStatus(string status)
    {
        if (status == "NEW")
        {
            return define::NEW;
        }
        else if (status == "PARTIALLY_FILLED")
        {
            return define::PARTIALLY_FILLED;
        }
        else if (status == "FILLED")
        {
            return define::FILLED;
        }
        else if (status == "CANCELED")
        {
            return define::CANCELED;
        }
        else if (status == "PENDING_CANCEL")
        {

            return define::PENDING_CANCEL;
        }
        else if (status == "REJECTED")
        {
            return define::REJECTED;
        }
        else if (status == "EXPIRED")
        {

            return define::EXPIRED;
        }

        return define::INVALID_ORDER_STATUS;
    }

    define::OrderSide BinanceApiWrapper::stringToSide(string side)
    {
        if (side == "BUY") {
            return define::BUY;
        } else if (side == "SELL") {
            return define::SELL;
        }

        return define::INVALID_SIDE;
    }

    string BinanceApiWrapper::sideToString(uint32_t side)
    {
        switch (side)
        {
        case define::BUY:
            return "BUY";
        case define::SELL:
            return "SELL";
        }

        return "";
    }

    string BinanceApiWrapper::orderTypeToString(uint32_t orderType)
    {
        switch (orderType)
        {
        case define::LIMIT:
            return "LIMIT";
        case define::MARKET:
            return "MARKET";
        case define::STOP_LOSS:
            return "STOP_LOSS";
        case define::STOP_LOSS_LIMIT:
            return "STOP_LOSS_LIMIT";
        case define::TAKE_PROFIT:
            return "TAKE_PROFIT";
        case define::TAKE_PROFIT_LIMIT:
            return "TAKE_PROFIT_LIMIT";
        case define::LIMIT_MAKER:
            return "LIMIT_MAKER";
        }

        return "";
    }

    string BinanceApiWrapper::timeInForceToString(uint32_t timeInForce)
    {
        switch (timeInForce)
        {
        case define::GTC:
            return "GTC";
        case define::IOC:
            return "IOC";
        case define::FOK:
            return "FOK";
        }

        return "";
    }

    pair<string, string> BinanceApiWrapper::parseToken(string symbol, define::OrderSide side)
    {
        auto data = GetSymbolData(symbol);
        if (side == define::SELL) {
            return make_pair(data.QuoteToken, data.BaseToken);
        } else {
            return make_pair(data.BaseToken, data.QuoteToken);
        }
    }
}