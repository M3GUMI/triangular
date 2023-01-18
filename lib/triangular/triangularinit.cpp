#include <sstream>

#include "triangular.h"
#include "lib/binance/binancecommondef.h"

using namespace binancecommondef;

namespace Triangular
{
    static void String2Double(const string &str, double &d)
    {
        std::istringstream s(str);
        s >> d;
    }

    Triangular::Triangular()
    {
        balanceTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(m_ioService, websocketpp::lib::asio::milliseconds(60 * 1000));
        balanceTimer->async_wait(std::bind(&Triangular::GetBalanceAndCheck, this));
        restapi_client = new BinanceRestApiWrapper::BinanceRestApiWrapper(m_ioService, access_key, secret_key);
        InitAllSymbol();

        m_mapSymbol2Base["BTCUSDT"] = make_pair("BTC", "USDT");
        m_mapSymbol2Base["DOGEUSDT"] = make_pair("DOGE", "USDT");
        m_mapSymbol2Base["DOGEBTC"] = make_pair("DOGE", "BTC");

        m_setBaseCoins.insert("BTC");
        m_setBaseCoins.insert("USDT");
        m_setBaseCoins.insert("DOGE");

        m_mapCoins2BeginAmount["BTC"] = 0.001;
        m_mapCoins2BeginAmount["USDT"] = 20;
        m_mapCoins2BeginAmount["ETH"] = 0.01;

        m_signalTrade = 0;
    }

    Triangular::~Triangular()
    {
        free(restapi_client);
    }

    void Triangular::ExchangeInfoHandler(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec)
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

        InitExchangeMap();
        CreateAndSubListenKey();
        // restapi_client->CreateOrder("BTCUSDT", Order_Side::SELL, Order_Type::LIMIT, TimeInFoce::IOC, 0.001, 16817, bind(&Triangular::OrderHandler, this, ::_1, ::_2));
    }

    // 拿所有交易对信息
    void Triangular::InitAllSymbol()
    {
        string requestLink = "https://api.binance.com/api/v3/exchangeInfo";
        restapi_client->BinanceMakeRequest({}, "GET", requestLink, "", std::bind(&Triangular::ExchangeInfoHandler, this, ::_1, ::_2), false);
        init_time = time(NULL);
    }

    void Triangular::InitExchangeMap()
    {
        unsigned index = 0;
        for (const auto &coin : m_setBaseCoins)
        {
            m_gExchangeMap.push_back(std::vector<TriangularExchangeMapNode>());
            m_mapCoins2Index[coin] = index;
            m_mapIndex2Coins[index] = coin;
            ++index;
        }
    }

    void Triangular::run()
    {
        m_ioService.run();
    }
}