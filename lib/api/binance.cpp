#include "./http/binancewebrestapiwrapper.h"
#include "./ws/binancedepthwebsocketclient.h"
#include "binance.h"


using namespace binance;


binance::binance::binance(std::string accessKey,std::string secretKey)
{
    

}

binance::binance::~binance()
{
}



void binance::binance::init()
{   
    binancewebrestapiwrapper wrapper = binancewebrestapiwrapper(ioService,accessKey,secretKey);
    wrapper.binanceInitSymbol();
    binancedepthwebsocketclient client = binancedepthwebsocketclient();
    client.CreateAndSubListenKey();
}

void binance::binance::createOrder(binancewebrestapiwrapper::CreateOrderReq sArgs)
{   
    binancewebrestapiwrapper wrapper = binancewebrestapiwrapper(ioService,accessKey,secretKey);
    wrapper.BcreateOrder(sArgs);
}

void binance::binance::cancelOrder(binancewebrestapiwrapper::cancelOrderReq cArgs)
{
    binancewebrestapiwrapper wrapper = binancewebrestapiwrapper(ioService,accessKey,secretKey);
    wrapper.BcancelOrder(cArgs);
}




