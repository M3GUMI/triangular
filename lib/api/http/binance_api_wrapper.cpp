#include <functional>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/reader.h>
#include <rapidjson/stringbuffer.h>
#include "binance_api_wrapper.h"
#include "defined/commondef.h"
#include "defined/defined.h"
#include "utils/utils.h"

using namespace std;
namespace HttpApi
{
    BinanceApiWrapper::BinanceApiWrapper(websocketpp::lib::asio::io_service *ioService)
    {
        BaseApiWrapper(ioService, "access_key", "secret_key");
    }

    BinanceApiWrapper::~BinanceApiWrapper()
    {
    }

    void BinanceApiWrapper::binanceInitSymbol()
    {
        /*string requestLink = "https://api.binance.com/api/v3/exchangeInfo";
        MakeRequest({}, "GET", requestLink, "", std::bind(&BinanceApiWrapper::ExchangeInfoHandler, this, placeholders::_1, placeholders::_2), false);
        init_time = time(NULL);*/
    }

    void BinanceApiWrapper::ExchangeInfoHandler(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec)
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
        if (exchangeInfoJson.HasMember("symbols"))
        {
            const auto &symbols = exchangeInfoJson["symbols"];
            for (unsigned i = 0; i < symbols.Size(); ++i)
            {
                std::string symbol = symbols[i].FindMember("symbol")->value.GetString();
                std::string baseAsset = symbols[i].FindMember("baseAsset")->value.GetString();
                std::string quoteAsset = symbols[i].FindMember("quoteAsset")->value.GetString();
                if (baseAsset == "NGN" || quoteAsset == "NGN")
                {
                    continue;
                }
                symbolMap[make_pair(baseAsset, quoteAsset)] = symbol;
                symbolMap[make_pair(quoteAsset, baseAsset)] = symbol;
                baseOfSymbol[symbol] = baseAsset;
                m_mapSymbol2Base[symbol] = make_pair(baseAsset, quoteAsset);
                m_setBaseCoins.insert(baseAsset);
                m_setBaseCoins.insert(quoteAsset);

                if (not symbols[i].HasMember("filters"))
                {
                    continue;
                }

                auto filters = symbols[i].FindMember("filters")->value.GetArray();
                for (uint32_t j = 0; j < filters.Size(); j++)
                {
                    if (not filters[j].HasMember("stepSize") || not filters[j].HasMember("filterType"))
                    {
                        continue;
                    }

                    std::string filterType = filters[j].FindMember("filterType")->value.GetString();

                    if (filterType != "LOT_SIZE")
                    {
                        continue;
                    }

                    std::string ticksize = filters[j].FindMember("stepSize")->value.GetString();
                    double double_tickSize = 0;
                    String2Double(ticksize, double_tickSize);
                    m_mapExchange2Ticksize[symbol] = double_tickSize;
                    break;
                }
            }
        }
    }

    int BinanceApiWrapper::CreateOrder(CreateOrderReq &req)
    {
        function<void(shared_ptr<HttpRespone> res, const ahttp::error_code &ec)> func;

        map<string, string> args;
        string uri = "https://api.binance.com/api/v3/order";
        pair<std::string, commondef::OrderSide> symbolSide;
        symbolSide = getSymbolSide(req.FromToken, req.ToToken);
        auto symbol = symbolSide.first;
        auto side = symbolSide.second;

        pair<double, double> priceQuantity;
        priceQuantity = getPriceQuantity(req, side);
        auto price = priceQuantity.first;
        auto quantity = priceQuantity.second;

        args["symbol"] = symbol;
        args["side"] = this->sideToString(side);
        args["type"] = this->orderTypeToString(req.OrderType);
        args["timestamp"] = std::to_string(time(NULL) * 1000);
        args["quantity"] = quantity;

        // 市价单 不能有下面这些参数
        if (req.OrderType != commondef::MARKET && req.OrderType != commondef::LIMIT_MAKER)
        {
            args["timeInForce"] = this->timeInForceToString(req.TimeInForce);
        }

        if (req.OrderType != commondef::MARKET)
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
            auto pair = this->getSymbolSide(symbol.first, symbol.second);
            this->cancelOrder("", pair.first);
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
                mapSymbolBalanceOrderStatus[ori_symbol] = 0;
            }
            return;
        }

        cout << __func__ << " " << __LINE__ << msg.c_str() << endl;

        // cout << __func__ << " " << __LINE__ << msg.c_str() << endl;
        // std::string symbol = order.FindMember("symbol")->value.GetString();

        mapSymbolBalanceOrderStatus[ori_symbol] = 0;
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

    void listenkeyKeepHandler()
    {
    }

    string BinanceApiWrapper::sideToString(uint32_t side)
    {
        switch (side)
        {
        case commondef::BUY:
            return "BUY";
        case commondef::SELL:
            return "SELL";
        }

        return "";
    }

    string BinanceApiWrapper::orderTypeToString(uint32_t orderType)
    {
        switch (orderType)
        {
        case commondef::LIMIT:
            return "LIMIT";
        case commondef::MARKET:
            return "MARKET";
        case commondef::STOP_LOSS:
            return "STOP_LOSS";
        case commondef::STOP_LOSS_LIMIT:
            return "STOP_LOSS_LIMIT";
        case commondef::TAKE_PROFIT:
            return "TAKE_PROFIT";
        case commondef::TAKE_PROFIT_LIMIT:
            return "TAKE_PROFIT_LIMIT";
        case commondef::LIMIT_MAKER:
            return "LIMIT_MAKER";
        }

        return "";
    }

    string BinanceApiWrapper::timeInForceToString(uint32_t timeInForce)
    {
        switch (timeInForce)
        {
        case commondef::GTC:
            return "GTC";
        case commondef::IOC:
            return "IOC";
        case commondef::FOK:
            return "FOK";
        }

        return "";
    }
}