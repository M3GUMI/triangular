#pragma once
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/reader.h>
#include <rapidjson/stringbuffer.h>

#include "lib/binance/binancerestapiwrapper.h"
#include "lib/binance/binancewebsocketwrapper.h"

#define access_key "ppp"
#define secret_key "ddd"

using namespace BinanceWebsocketWrapper;

namespace Triangular
{

    class TriangularExchangeMapNode
    {
    public:
        TriangularExchangeMapNode() {}
        TriangularExchangeMapNode(uint32_t t, double p, double f, uint64_t l, uint32_t s) : to_point(t), price(p), fee(f), lastUpdateId(l), side(s) {}
        ~TriangularExchangeMapNode() {}

        uint32_t to_point;
        double price;
        double fee;
        uint64_t lastUpdateId;
        uint32_t side;
    };

    class Triangular
    {
    public:
        Triangular();
        ~Triangular();

        void run();
        void GetBalanceAndCheck();
        void GetBalanceAndCheckNoWait();
        void AddDepthSubscirbe(std::string exchangeName, bool b_run_path_cal);

    private:
        // 初始化图和所有交易对
        void InitAllSymbol();
        void InitExchangeMap();
        void ExchangeInfoHandler(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec);

        // 更新图的操作
        void UpdateExhangeMap(std::string symbol, double bid, double ask, double fee, uint64_t lastUpdateId, bool &is_price_change);
        void UpdateExhangeMapOneStep(uint32_t from, uint32_t to, double price, double fee, uint64_t lastUpdateId, uint32_t side, bool &is_price_change);

        // depth订阅回调
        void DepthMsgHandler(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg, std::string exchangeName, bool b_run_path_cal);
        void OrderHandler(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec);

        void WSMsgHandler(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg);

        void CreateAndSubListenKey();
        void CreateListkeyHandler(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec);
        void KeeyListenKey();
        void listenkeyKeepHandler();
        void ExecutionReportHandler(const rapidjson::Document &msg);

        vector<vector<uint32_t>> TriangularKiller(std::string begin, std::string end);
        vector<uint32_t> TriangularBestPathFinder(std::string begin, std::string end);
        vector<uint32_t> GetTriangularKillerPath(uint32_t start, uint32_t end, std::map<uint32_t, uint32_t> &mapIndex2PerPath);
        vector<uint32_t> GetBestPath(uint32_t start, uint32_t end, std::map<uint32_t, uint32_t> &mapIndex2PerPath);
        void ShowPath(vector<uint32_t> path);

        void TriangularCreateOrder(std::string from, std::string to, double amount);
        bool IsStatic(std::string coin);

        void AccountInfoHandler(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec);
        void TryRebalance(std::string to, std::string from, double amount);
        void TryRebalanceOrderHandler(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec);
        void CancelAllCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec, std::string ori_symbol);

        void CancelAllOrder(std::string symbol);

        std::set<std::string> m_setBaseCoins;                                        // 所有交易对的基础货币
        std::map<std::string, uint32_t> m_mapCoins2Index;                            // 货币名到下标
        std::map<uint32_t, std::string> m_mapIndex2Coins;                            // 下标到货币名
        std::map<std::string, std::pair<std::string, std::string>> m_mapSymbol2Base; // 交易对到货币名
        // const std::set<std::string> m_setStaticCoins = {"BTC", "USDT", "btc", "usdt"}; // 稳定币
        std::map<std::string, double> m_mapExchange2Ticksize;

        websocketpp::lib::asio::io_service m_ioService;
        BinanceRestApiWrapper::BinanceRestApiWrapper *restapi_client;
        BinanceWebsocketWrapper::BinanceWebsocketWrapper *ws_client;
        std::map<std::string, BinanceWebsocketWrapper::BinanceWebsocketWrapper *> depth_ws_client;
        std::string m_listenKey;

        // 图的内存存储结构
        std::vector<std::vector<TriangularExchangeMapNode>> m_gExchangeMap;

        std::map<string, double> m_mapCoins2BeginAmount; // 初始第一波交易多少
        std::shared_ptr<websocketpp::lib::asio::steady_timer> listenkeyKeepTimer;
        std::shared_ptr<websocketpp::lib::asio::steady_timer> balanceTimer;

        uint32_t m_signalTrade;
        uint32_t init_time = 0;
        uint32_t print_counter = 0;

        map<std::string, uint32_t> mapSymbolBalanceOrderStatus;
    };
}