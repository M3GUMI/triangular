#include "webrestapiwrapper.h"

using namespace webrestapiwrapper;

class binancewebrestapiwrapper : public webrestapiwrapper
{

public:

    binancewebrestapiwrapper(websocketpp::lib::asio::io_service &ioService, std::string accessKey, std::string secretKey); // 公钥 密钥配置构造web_client
    binancewebrestapiwrapper();
    ~binancewebrestapiwrapper();


    struct CreateOrderReq
    {
        std::string FromToken;
        double FromPrice;
        double FromQuantity;
        std::string ToToken;
        double ToPrice;
        double ToQuantity;
        std::string OrderType;
        std::string TimeInForce;
    };

    struct calcelOrderArgs
    {
        bool CancelAll;
        vector<pair<std::string, std::string>> pairs;
        uint64_t OrderId;
    };
    // struct pairs
    // {
    //     std::string token0;
    //     std::string token1;
    // }pairs[];
    // 初始化
    void binanceInitSymbol();
    void ExchangeInfoHandler(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec);


    //发起购买方法
    void createOrder(CreateOrderReq sArgs, std::function<void(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec)> func);
    void OrderHandler(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec);
    std::string BcreateOrder(CreateOrderReq sArgs);

    //发起取消订单方法
    void cancelOrder(std::string symbol);
    void CancelAllCallback(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec, std::string ori_symbol);

    //获取symbol对
    
    pair<std::string, std::string> getSymbolSize(std::string token0, std::string token1);
    pair<double, double>selectPriceQuantity(CreateOrderReq args, std::string symbol);
    //symobl储存
    std::map<std::string, std::string> baseOfSymbol;
    std::map<pair<std::string, std::string>, std::string> symbolMap;
    std::map<std::string, uint32_t> mapSymbolBalanceOrderStatus;

    // 签名
    std::map<std::string, std::string> sign(std::map<std::string, std::string> &args, std::string &query, bool need_sign); // 需要根据特定重写
    std::string toURI(const map<std::string, std::string> &mp);                                                            // 需要重写
    template <typename T, typename outer>
    std::string hmac(const std::string &key, const std::string &data, T evp, outer filter);
    std::string accessKey, secretKey;

    // 发出请求
    void MakeRequest(map<std::string, std::string> args, std::string method, std::string uri, string data, std::function<void(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec)> func, bool need_sign);



    uint32_t init_time = 0;

    std::set<std::string> m_setBaseCoins;                                        // 所有交易对的基础货币
    std::map<std::string, uint32_t> m_mapCoins2Index;                            // 货币名到下标
    std::map<uint32_t, std::string> m_mapIndex2Coins;                            // 下标到货币名
    std::map<std::string, std::pair<std::string, std::string>> m_mapSymbol2Base; // 交易对到货币名
    std::map<std::string, double> m_mapExchange2Ticksize;
    websocketpp::lib::asio::io_service *ioService;
};