#include <rapidjson/document.h>
#include "define/define.h"
#include "utils/utils.h"
#include "binance_api_wrapper.h"

using namespace std;
namespace HttpApi
{
    BinanceApiWrapper::BinanceApiWrapper(websocketpp::lib::asio::io_service &ioService) : BaseApiWrapper(ioService, GetAccessKey().first, GetAccessKey().second)
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
        req.callback = bind(&BinanceApiWrapper::initBinanceSymbolCallback, this, placeholders::_1, placeholders::_2);
    }

    void BinanceApiWrapper::initBinanceSymbolCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec)
    {
        if (res == nullptr || res->payload().empty())
        {
            cout << __func__ << " " << __LINE__ << " get_balance error " << endl;
            return;
        }

        auto &msg = res->payload();
        if (res->http_status() != 200)
        {
            cout << __func__ << " " << __LINE__ << "***** get_balance http status error *****" << res->http_status() << " msg: " << msg << endl;
            return;
        }

        rapidjson::Document exchangeInfoJson;
        exchangeInfoJson.Parse(msg.c_str());
        if (!exchangeInfoJson.HasMember("symbols"))
        {
            return;
        }

        const auto &symbols = exchangeInfoJson["symbols"];
        for (unsigned i = 0; i < symbols.Size(); i++)
        {
            double double_tickSize = 0;
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
                    String2Double(ticksize, double_tickSize);
                    break;
                }
            }

            BinanceSymbolData data;
            data.Symbol = symbol;
            data.BaseToken = baseAsset;
            data.QuoteToken = quoteAsset;
            data.TicketSize = double_tickSize;

            symbolMap[baseAsset + quoteAsset] = data;
            symbolMap[quoteAsset + baseAsset] = data;
            baseCoins.insert(baseAsset);
            baseCoins.insert(quoteAsset);
        }
    }

    BinanceSymbolData& BinanceApiWrapper::GetSymbolData(std::string token0, std::string token1)
    {
        auto symbol = token0 + token1;
        if (symbolMap.count(symbol) != 1)
        {
            BinanceSymbolData data;
            cout << "not found error" << endl;
            return data;
        }

        BinanceSymbolData data = symbolMap[symbol];
        return data;
    }

    BinanceSymbolData& BinanceApiWrapper::GetSymbolData(std::string symbol)
    {
        if (symbolMap.count(symbol) != 1)
        {
            BinanceSymbolData data;
            cout << "not found error" << endl;
            return data;
        }

        BinanceSymbolData data = symbolMap[symbol];
        return data;
    }

    string BinanceApiWrapper::GetSymbol(std::string token0, std::string token1)
    {
        auto symbol = token0 + token1;
        if (symbolMap.count(symbol) != 1)
        {
            cout << "not found error" << endl;
            return "";
        }

        BinanceSymbolData data = symbolMap[symbol];
        return data.Symbol;
    }

    define::OrderSide BinanceApiWrapper::GetSide(std::string token0, std::string token1)
    {
        auto symbol = token0 + token1;
        if (symbolMap.count(symbol) != 1)
        {
            cout << "not found error" << endl;
            return define::UNKNOWN;
        }

        define::OrderSide side;
        BinanceSymbolData data = symbolMap[symbol];
        data.BaseToken == token0 ? side = define::SELL : side = define::BUY;
        return side;
    }

    int BinanceApiWrapper::CreateOrder(CreateOrderReq &req)
    {
        function<void(shared_ptr<HttpRespone> res, const ahttp::error_code &ec)> func;

        map<string, string> args;
        string uri = "https://api.binance.com/api/v3/order";
        pair<std::string, define::OrderSide> symbolSide;
        auto symbol = GetSymbol(req.FromToken, req.ToToken);
        auto side = GetSide(req.FromToken, req.ToToken);

        pair<double, double> priceQuantity;
        priceQuantity = GetPriceQuantity(req, side);
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

        cout << "Create order " << symbol << " " << side << " " << price << " " << quantity << endl;

        ApiRequest apiReq;
        apiReq.args = args;
        apiReq.method = "POST";
        apiReq.uri = uri;
        apiReq.data = "";
        apiReq.sign = true;
        apiReq.callback = bind(&BinanceApiWrapper::createOrderCallback, this, placeholders::_1, placeholders::_2),
        BaseApiWrapper::MakeRequest(apiReq);
        return 0;
    }

    void BinanceApiWrapper::createOrderCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec)
    {
        if (res == nullptr || res->payload().empty())
        {
            cout << __func__ << " " << __LINE__ << " get_balance error " << endl;
            return;
        }

        auto &msg = res->payload();
        rapidjson::Document order;
        order.Parse(msg.c_str());
        if (res->http_status() != 200)
        {
            cout << __func__ << " " << __LINE__ << "***** get_balance http status error *****" << res->http_status() << " msg: " << msg << endl;
            int code = order.FindMember("code")->value.GetInt();
            std::string msg = order.FindMember("msg")->value.GetString();
            if (code == -2010 && msg == "Account has insufficient balance for requested action.")
            {
                cout << "余额不足" << endl;
            }
            return;
        }

        return;
    }

    void BinanceApiWrapper::cancelOrder(string orderId, string symbol)
    {
        map<string, string> args;
        string uri = "https://api.binance.com/api/v3/openOrders";
        args["symbol"] = symbol;
        args["timestamp"] = std::to_string(time(NULL) * 1000);
        args["recvWindow"] = "50000";
        args["newClientOrderId"] = this->GetClientOrderId(orderId);

        ApiRequest req;
        req.args = args;
        req.method = "DELETE";
        req.uri = uri;
        req.data = "";
        req.sign = true;
        req.callback = bind(&BinanceApiWrapper::cancelOrderCallback, this, placeholders::_1, placeholders::_2, symbol);

        MakeRequest(req);
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
        if (res == nullptr || res->payload().empty())
        {
            cout << __func__ << " " << __LINE__ << " cancel all error " << endl;
            return;
        }

        auto &msg = res->payload();
        rapidjson::Document order;
        order.Parse(msg.c_str());

        if (res->http_status() != 200)
        {
            cout << __func__ << " " << __LINE__ << "***** cancel all http status error *****" << res->http_status() << " msg: " << msg << endl;
            int code = order.FindMember("code")->value.GetInt();
            std::string msg = order.FindMember("msg")->value.GetString();

            // 找不到订单了
            if (code == -2011 && msg == "Unknown order sent.")
            {
                // todo 改成回调通知订单管理层
                // mapSymbolBalanceOrderStatus[ori_symbol] = 0;
            }
            return;
        }

        cout << __func__ << " " << __LINE__ << msg.c_str() << endl;

        // todo 改成回调通知订单管理层
        // mapSymbolBalanceOrderStatus[ori_symbol] = 0;
    }

    void BinanceApiWrapper::CreateListenKey(string listenKey, function<void(int errCode, string listenKey)> callback)
    {
        string uri = "https://api.binance.com/api/v3/userDataStream";
        ApiRequest req;
        req.args = {};
        req.method = "POST";
        req.uri = uri;
        req.data = "";
        req.sign = true;
        req.callback = bind(&BinanceApiWrapper::createListkeyCallback, this, placeholders::_1, placeholders::_2, callback);

        if (listenKey.size() == 0)
        {
            req.args["listenKey"] = listenKey;
            req.method = "PUT";
        }

        this->MakeRequest(req);
    }

    void BinanceApiWrapper::createListkeyCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec, function<void(int errCode, string listenKey)> callback)
    {
        if (res == nullptr || res->payload().empty())
        {
            cout << __func__ << " " << __LINE__ << " CreateListkey error " << endl;
            return callback(1, "");
        }

        auto &msg = res->payload();
        if (res->http_status() != 200)
        {
            cout << __func__ << " " << __LINE__ << "***** CreateListkey http status error *****" << res->http_status() << " msg: " << msg << endl;
            return callback(1, "");
        }

        rapidjson::Document listenKeyInfo;
        listenKeyInfo.Parse(msg.c_str());

        if (listenKeyInfo.HasMember("listenKey"))
        {
            auto listenKey = listenKeyInfo.FindMember("listenKey")->value.GetString();
            return callback(0, listenKey);
        }

        return callback(2, "");
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
}