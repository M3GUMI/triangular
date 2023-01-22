#include "webrestapiwrapper.h"
#include "../commondef.h"

using namespace commondef;
namespace webrestapiwrapper
{
    webrestapiwrapper::webrestapiwrapper(websocketpp::lib::asio::io_service &ioService, string accessKey, string secretKey)
    {
    }
    webrestapiwrapper::webrestapiwrapper()
    {
    }
    webrestapiwrapper::~webrestapiwrapper()
    {
    }

    void webrestapiwrapper::MakeRequest(map<std::string, std::string> args, std::string method, std::string uri, string data, std::function<void(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec)> func, bool need_sign)
    {
        

    }
    void webrestapiwrapper::CreateOrder(std::string symbol, uint32_t side, uint32_t order_type, uint32_t timeInForce, double quantity, double price, std::function<void(std::shared_ptr<HttpRespone> res, const ahttp::error_code &ec)> func, std::string uri)
    {
         std::string strSide, strType, strTimeInForce;

        switch (side)
        {
        case Order_Side::BUY:
            strSide = "BUY";
            break;
        case Order_Side::SELL:
            strSide = "SELL";
            break;
        }

        switch (order_type)
        {
        case Order_Type::LIMIT:
            strType = "LIMIT";
            break;
        case Order_Type::MARKET:
            strType = "MARKET";
            break;
        case Order_Type::STOP_LOSS:
            strType = "STOP_LOSS";
            break;
        case Order_Type::STOP_LOSS_LIMIT:
            strType = "STOP_LOSS_LIMIT";
            break;
        case Order_Type::TAKE_PROFIT:
            strType = "TAKE_PROFIT";
            break;
        case Order_Type::TAKE_PROFIT_LIMIT:
            strType = "TAKE_PROFIT_LIMIT";
            break;
        case Order_Type::LIMIT_MAKER:
            strType = "LIMIT_MAKER";
            break;
        }

        switch (timeInForce)
        {
        case TimeInFoce::GTC:
            strTimeInForce = "GTC";
            break;
        case TimeInFoce::IOC:
            strTimeInForce = "IOC";
            break;
        case TimeInFoce::FOK:
            strTimeInForce = "FOK";
            break;
        }

        map<string, string> args;
        args["symbol"] = symbol;
        args["timestamp"] = std::to_string(time(NULL) * 1000);
        args["side"] = strSide;
        args["type"] = strType;
        args["quantity"] = to_string(quantity);

         // 市价单 不能有下面这些参数
        if (order_type != Order_Type::MARKET && order_type != Order_Type::LIMIT_MAKER)
        {
            args["timeInForce"] = strTimeInForce;
            args["price"] = to_string(price);
        }

        if (order_type != Order_Type::MARKET)
        {
            args["price"] = to_string(price);
        }

        cout << "Create order" << " " << symbol << " " << side << " " << price << " " << quantity << endl;
    
        MakeRequest(args, "POST", uri, "", func, true);
    }
   
    static void String2Double(const string &str, double &d)
    {
        std::istringstream s(str);
        s >> d;
    }

     static uint64_t getTime()
    {

        timeb t;
        ftime(&t); // 获取毫秒
        time_t curr =  t.time * 1000 + t.millitm;
        uint64_t time = ((uint64_t)curr);
        return time;
    }
}