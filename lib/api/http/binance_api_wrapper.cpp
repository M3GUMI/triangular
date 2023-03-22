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

    void BinanceApiWrapper::SubscribeSymbolReady(function<void(vector<BinanceSymbolData> &data)> callback)
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
            spdlog::error("func: {}, err: {}", "initBinanceSymbolCallback", WrapErr(define::ErrorInvalidResp));
            return;
        }

        vector<BinanceSymbolData> symbolData;
        const auto &symbols = exchangeInfoJson["symbols"];
        for (unsigned i = 0; i < symbols.Size(); i++)
        {
            double doubleStepSize = 0, doubleTicketSize = 0, doubleMinNotional = 0;
            string symbol = symbols[i].FindMember("symbol")->value.GetString();
            string baseAsset = symbols[i].FindMember("baseAsset")->value.GetString();
            string quoteAsset = symbols[i].FindMember("quoteAsset")->value.GetString();
            if (baseAsset == "NGN" || quoteAsset == "NGN")
            {
                continue;
            }

            if (symbols[i].HasMember("filters")) {
                auto filters = symbols[i].FindMember("filters")->value.GetArray();
                for (uint32_t j = 0; j < filters.Size(); j++) {
                    if (not filters[j].HasMember("filterType")) {
                        continue;
                    }

                    string filterType = filters[j].FindMember("filterType")->value.GetString();
                    if (filterType == "LOT_SIZE" && filters[j].HasMember("stepSize")) {
                        string stepSize = filters[j].FindMember("stepSize")->value.GetString();
                        doubleStepSize = String2Double(stepSize);
                        if (doubleStepSize == 0) {
                            doubleStepSize = 1;
                        }
                    } else if (filterType == "PRICE_FILTER" && filters[j].HasMember("tickSize")) {
                        string ticketSize = filters[j].FindMember("tickSize")->value.GetString();
                        doubleTicketSize = String2Double(ticketSize);
                    } else if (filterType == "MIN_NOTIONAL" && filters[j].HasMember("minNotional")) {
                        string minNotional = filters[j].FindMember("minNotional")->value.GetString();
                        doubleMinNotional = String2Double(minNotional);
                    }
                }
            }

            BinanceSymbolData data;
            data.Symbol = symbol;
            data.BaseToken = baseAsset;
            data.QuoteToken = quoteAsset;
            data.StepSize = doubleStepSize;
            data.TicketSize = doubleTicketSize;
            data.MinNotional = doubleMinNotional;

            symbolData.push_back(data);
            symbolMap[baseAsset + quoteAsset] = data;
            symbolMap[quoteAsset + baseAsset] = data;
            baseCoins.insert(baseAsset);
            baseCoins.insert(quoteAsset);
        }

        spdlog::info("func: {}, msg: {}", "InitBinanceSymbol", "load symbol data success");
        for (auto func : this->symbolReadySubscriber)
        {
            func(symbolData);
        }
    }

    BinanceSymbolData& BinanceApiWrapper::GetSymbolData(const std::string& token0, const std::string& token1)
    {
        auto symbol = toUpper(token0 + token1);
        if (symbolMap.count(symbol) != 1)
        {
            spdlog::error("func: {}, msg: {}, err: {}", "GetSymbolData", "not found symbol", define::ErrorDefault);
            exit(EXIT_FAILURE);
        }

        return symbolMap[symbol];
    }

    BinanceSymbolData& BinanceApiWrapper::GetSymbolData(const std::string& symbol)
    {
        if (symbolMap.count(toUpper(symbol)) != 1)
        {
            spdlog::error("func: {}, msg: {}, err: {}", "GetSymbolData", "not found symbol", define::ErrorDefault);
            exit(EXIT_FAILURE);
        }

        return symbolMap[symbol];
    }

    string BinanceApiWrapper::GetSymbol(const std::string& token0, const std::string& token1)
    {
        auto symbol = toUpper(token0 + token1);
        if (symbolMap.count(symbol) != 1)
        {
            spdlog::error("func: {}, msg: {}, err: {}", "GetSymbolData", "not found symbol", define::ErrorDefault);
            exit(EXIT_FAILURE);
        }

        BinanceSymbolData data = symbolMap[symbol];
        return data.Symbol;
    }

    define::OrderSide BinanceApiWrapper::GetSide(const std::string& token0, const std::string& token1)
    {
        auto symbol = toUpper(token0 + token1);
        if (symbolMap.count(symbol) != 1)
        {
            spdlog::error("func: {}, msg: {}, err: {}", "GetSymbolData", "not found symbol", define::ErrorDefault);
            exit(EXIT_FAILURE);
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
            data.Token = "USDT";
            data.Free = 20;
            data.Locked = 0;
            info.Balances.push_back(data);
            spdlog::debug("func: {}, msg: {}", "GetAccountInfo", "mock account_info");
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
            spdlog::error("func: {}, msg: {}, err: {}", "accountInfoHandler", "no balance data", define::ErrorInvalidResp);
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
            data.Free = String2Double(free);
            data.Locked = String2Double(locked);
            info.Balances.push_back(data);
        }

        spdlog::info("func: {}, msg: {}", "GetAccountInfo", "get account_info success");
        return callback(info, 0);
    }

    int BinanceApiWrapper::GetOpenOrder(string symbol)
    {
        uint64_t now = time(NULL);

        ApiRequest req;
        req.args["timestamp"] = to_string(now * 1000);
        req.method = "GET";
        req.uri = "https://api.binance.com/api/v3/openOrders";
        req.data = "";
        req.sign = true;
        auto apiCallback = bind(&BinanceApiWrapper::getOpenOrderCallback, this, ::_1, ::_2);

        this->MakeRequest(req, apiCallback, true);
        return 0;
    }

    void BinanceApiWrapper::getOpenOrderCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec)
    {
        spdlog::info("orders: {}", res->payload());
    }

    int BinanceApiWrapper::GetUserAsset(function<void(double btc)> callback)
    {
        uint64_t now = time(NULL);

        ApiRequest req;
        req.args["timestamp"] = to_string(now * 1000);
        req.args["needBtcValuation"] = "true";
        req.method = "POST";
        req.uri = "https://api.binance.com/sapi/v3/asset/getUserAsset";
        req.data = "";
        req.sign = true;
        auto apiCallback = bind(&BinanceApiWrapper::getUserAssetHandler, this, ::_1, ::_2, callback);

        this->MakeRequest(req, apiCallback);
        return 0;
    }

    void BinanceApiWrapper::getUserAssetHandler(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec, function<void(double btc)> callback)
    {
        rapidjson::Document data;
        if(res == nullptr){
            return;
        }
        data.Parse(res->payload().c_str());

        double btcAsset = 0;
        auto arr = data.GetArray();
        for (int i = 0;i < arr.Size();i++) {
            string asset = arr[i].FindMember("btcValuation")->value.GetString();
            btcAsset += String2Double(asset);
        }

        callback(btcAsset);
    }

    int BinanceApiWrapper::CreateOrder(OrderData &req, function<void(OrderData& data, int err)> callback) {
        if (req.OrderId == 0) {
            spdlog::error("func: CreateOrder, orderId: {}, err: {}", req.OrderId, WrapErr(define::ErrorInvalidParam));
            return define::ErrorInvalidParam;
        }
        if (req.BaseToken.empty() || req.QuoteToken.empty()) {
            spdlog::error(
                    "func: CreateOrder, fromToken: {}, toToken: {}, side: {}, err: {}",
                    req.BaseToken,
                    req.QuoteToken,
                    req.Side,
                    WrapErr(define::ErrorInvalidParam));
            return define::ErrorInvalidParam;
        }
        if (req.Price == 0 || req.Quantity == 0) {
            spdlog::error(
                    "func: CreateOrder, price: {}, quantity: {}, err: {}",
                    req.Price,
                    req.Quantity,
                    WrapErr(define::ErrorInvalidParam));
            return define::ErrorInvalidParam;
        }

        map<string, string> args;
        string uri = "https://api.binance.com/api/v3/order";

        // ticketSize校验
        auto symbolData = GetSymbolData(req.BaseToken, req.QuoteToken);
        uint32_t tmpPrice = req.Price / symbolData.TicketSize;
        req.Price = tmpPrice * symbolData.TicketSize;

        // stepSize校验
        uint32_t tmpQuantity = req.Quantity / symbolData.StepSize;
        req.Quantity = tmpQuantity * symbolData.StepSize;

        if (req.Quantity == 0) {
            req.OrderStatus = define::EXPIRED;
            return define::ErrorLessTicketSize;
        }

        if (req.GetNationalQuantity() < symbolData.MinNotional) {
            req.OrderStatus = define::EXPIRED;
            return define::ErrorLessMinNotional;
        }

        if (req.OrderType == define::LIMIT && req.TimeInForce == define::IOC) {
            args["newOrderRespType"] = "RESULT";
        }
        args["symbol"] = symbolData.Symbol;
        args["side"] = sideToString(req.Side);
        args["type"] = orderTypeToString(req.OrderType);
        args["timestamp"] = std::to_string(time(NULL) * 1000);
        args["quantity"] =  FormatDouble(req.Quantity);

        // 市价单 不能有下面这些参数
        if (req.OrderType != define::MARKET && req.OrderType != define::LIMIT_MAKER) {
            args["timeInForce"] = timeInForceToString(req.TimeInForce);
        }

        if (req.OrderType != define::MARKET) {
            args["price"] = FormatDouble(req.Price);
        }

        spdlog::debug(
                "func: CreateOrder, symbol: {}, price: {}, quantity: {}, side: {}, orderType: {}",
                args["symbol"], req.Price, args["quantity"], args["side"], args["type"]
        );
        ApiRequest apiReq;
        apiReq.args = args;
        apiReq.method = "POST";
        apiReq.uri = uri;
        apiReq.data = "";
        apiReq.sign = true;
        auto apiCallback = bind(
                &BinanceApiWrapper::createOrderCallback,
                this,
                placeholders::_1,
                placeholders::_2,
                req,
                callback
        );
        this->MakeRequest(apiReq, apiCallback);
        return 0;
    }

    void BinanceApiWrapper::createOrderCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec, OrderData &req, function<void(OrderData& data, int err)> callback) {
        OrderData data;
        if (conf::EnableMock)
        {
            data.OrderId = req.OrderId;
            data.UpdateTime = GetNowTime();
            data.OrderStatus = define::NEW;
            data.Price = req.Price;
            data.Quantity = req.Quantity;
            data.Side = req.Side;
            data.OrderType = req.OrderType;
            data.TimeInForce = req.TimeInForce;


            if (data.OrderType == define::LIMIT || data.OrderType == define::MARKET)
            {
                data.OrderStatus = define::FILLED;
                data.ExecutePrice = req.Price;
                data.ExecuteQuantity = req.Quantity;
                data.CummulativeQuoteQuantity = RoundDouble(data.Quantity * data.ExecutePrice);
            }

            spdlog::debug("func: {}, msg: {}", "createOrderCallback", "mock create_order");
            return callback(data, 0);
        }

        if (auto checkResult = this->CheckRespWithCode(res); checkResult.Err > 0) {
            if (checkResult.Code == -2010 &&
                checkResult.Msg == "Account has insufficient balance for requested action.") {
                spdlog::error("func: {}, err: {}", "CreateOrder", define::ErrorInsufficientBalance);
                return callback(data, define::ErrorInsufficientBalance);
            }

            return callback(data, checkResult.Err);
        }

        rapidjson::Document order;
        order.Parse(res->payload().c_str());

        string clientOrderId = order.FindMember("clientOrderId")->value.GetString();
        orderIdMap[req.OrderId] = clientOrderId;
        outOrderIdMap[clientOrderId] = req.OrderId;

        auto symbolData = GetSymbolData(order["symbol"].GetString());
        uint64_t transactTime = order["transactTime"].GetUint64();

        data.OrderId = this->outOrderIdMap[order["clientOrderId"].GetString()];
        data.BaseToken = symbolData.BaseToken;
        data.QuoteToken = symbolData.QuoteToken;
        data.UpdateTime = transactTime;

        if (req.OrderType == define::LIMIT_MAKER && req.TimeInForce == define::GTC) {
            // maker下单ack
            data.OrderStatus = define::NEW;
        } else {
            // taker下单
            double price = String2Double(order["price"].GetString());
            double quantity = String2Double(order["origQty"].GetString());
            double executeQuantity = String2Double(order["executedQty"].GetString());
            double cummulativeQuoteQty = String2Double(order["cummulativeQuoteQty"].GetString());

            data.OrderStatus = stringToOrderStatus(order["status"].GetString());
            data.Side = stringToSide(order["side"].GetString());

            // data.Price = price; 使用内存数据
            data.Quantity = quantity;

            data.ExecutePrice = price;
            data.ExecuteQuantity = executeQuantity;
            data.CummulativeQuoteQuantity = cummulativeQuoteQty;
        }

        spdlog::debug(
                "func: createOrderCallback, orderId: {}, status: {}, base: {}, quote: {}, side: {}, originQuantity: {}, executePrice: {}, executeQuantity: {}",
                data.OrderId,
                data.OrderStatus,
                data.BaseToken,
                data.QuoteToken,
                data.Side,
                data.Quantity,
                data.ExecutePrice,
                data.ExecuteQuantity
        );
        return callback(data, 0);
    }

    void BinanceApiWrapper::cancelOrder(uint64_t orderId, string symbol)
    {
        map<string, string> args;
        string uri = "https://api.binance.com/api/v3/openOrders";
        args["timestamp"] = std::to_string(time(NULL) * 1000);
        args["recvWindow"] = "50000";
        if (not symbol.empty()) {
            args["symbol"] = symbol;
        }
        if (orderId > 0) {
            args["newClientOrderId"] = GetOutOrderId(orderId);
        }

        ApiRequest req;
        req.args = args;
        req.method = "DELETE";
        req.uri = uri;
        req.data = "";
        req.sign = true;
        auto callback = bind(&BinanceApiWrapper::cancelOrderCallback, this, placeholders::_1, placeholders::_2, symbol);

        MakeRequest(req, callback);
    }

    void BinanceApiWrapper::CancelOrder(uint64_t orderId)
    {
        this->cancelOrder(orderId, "");
    }

    void BinanceApiWrapper::CancelOrderSymbol(string token0, string token1)
    {
        auto symbolStr = this->GetSymbol(token0, token1);
        this->cancelOrder(0, symbolStr);
    }

    void BinanceApiWrapper::CancelOrderSymbols(vector<pair<string, string>> symbols)
    {
        for (auto symbol : symbols)
        {
            auto symbolStr = this->GetSymbol(symbol.first, symbol.second);
            this->cancelOrder(0, symbolStr);
        }
    }

    void BinanceApiWrapper::CancelOrderAll()
    {
        // todo 增加账号symbol扫描
        vector<pair<string, string>> symbols;
        this->CancelOrderSymbols(symbols);
    }

    void BinanceApiWrapper::cancelOrderCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec, std::string ori_symbol)
    {
        if (conf::EnableMock) {
            return;
        }

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

        if (listenKey.size() != 0)
        {
            req.args["listenKey"] = listenKey;
            req.method = "PUT";
        }

        this->MakeRequest(req, apiCallback);
    }

    void BinanceApiWrapper::createListkeyCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec, function<void(string listenKey, int err)> callback)
    {
        if (conf::EnableMock) {
            return callback("mock", 0);
        }

        if (auto err = this->CheckResp(res); err > 0)
        {
            spdlog::error("func: createListkeyCallback, err: {}, resp: {}", WrapErr(err), res->payload());
            return callback("", err);
        }

        rapidjson::Document listenKeyInfo;
        listenKeyInfo.Parse(res->payload().c_str());

        if (!listenKeyInfo.HasMember("listenKey"))
        {
            spdlog::error("func: createListkeyCallback, err: {}, resp: {}", WrapErr(ErrorInvalidResp), res->payload());
            return callback("", define::ErrorInvalidResp);
        }

        auto listenKey = listenKeyInfo.FindMember("listenKey")->value.GetString();
        return callback(listenKey, 0);
    }
}