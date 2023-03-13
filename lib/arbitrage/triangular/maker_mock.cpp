#include "iostream"
#include <sstream>
#include "maker_mock.h"

using namespace boost::asio;
using namespace std;
using namespace Arbitrage;

namespace makerMock{

    MakerMock::MakerMock(
            Pathfinder::Pathfinder &pathfinder,
            Arbitrage::TriangularArbitrage triangular)
    : triangular(triangular),pathfinder(pathfinder)
    {}


    MakerMock::~MakerMock()  = default;


    void MakerMock::tradeNodeHandler(map<u_int64_t, Pathfinder::Node*> tradeNodeMap){
        mockTradeNodeMap = tradeNodeMap;
        mockOrderMap = triangular.getOrderMap();
        mockTrader(mockTradeNodeMap, mockOrderMap);
    };

    void MakerMock::mockTrader(map<u_int64_t, Pathfinder::Node*> mockTradeNodeMap, map<uint64_t, OrderData*> mockOrderMap){
        //将ordermap里面的数据拿出来进行核对，只要符合那么便将订单完成
        for (auto it = mockOrderMap.begin(); it != mockOrderMap.end(); it++) {

            int key = it->first;
            define::OrderSide Side = mockOrderMap[key]->Side;
            int fromIndex = pathfinder.tokenToIndex[mockOrderMap[key]->GetFromToken()];
            int toIndex = pathfinder.tokenToIndex[mockOrderMap[key]->GetToToken()];
            double price = mockOrderMap[key]->Price;
            double quantity = mockOrderMap[key]->Quantity;
            double tradePrice = mockTradeNodeMap[fromIndex + toIndex]->GetOriginPrice(fromIndex, toIndex);
            double tradeQuantity = mockTradeNodeMap[fromIndex + toIndex]->GetQuantity(fromIndex, toIndex);
            if (tradePrice == 0 || tradePrice >= price){
                return;
            }
            if (quantity == 0){
                return;
            }
            if (Side == define::BUY && price >= tradePrice && price != 0 ){
                if (tradeQuantity >= quantity && tradeQuantity != 0)
                {
                    mockOrderMap[key]->OrderStatus = define::FILLED;
                    mockOrderMap[key]->ExecuteQuantity = tradeQuantity;
                    mockOrderMap[key]->ExecutePrice = tradePrice;
                    //剩余数量清零
                    mockOrderMap[key]->Quantity = 0;
                }
                if (tradeQuantity <= quantity && tradeQuantity != 0)
                {
                    mockOrderMap[key]->OrderStatus = define::PARTIALLY_FILLED;
                    mockOrderMap[key]->ExecuteQuantity = quantity;
                    mockOrderMap[key]->ExecutePrice = tradePrice;
                    mockOrderMap[key]->Quantity -= tradeQuantity;
                }
            }
            if (Side == define::SELL && price <= tradePrice && price != 0 ){
                if (tradeQuantity >= quantity && tradeQuantity != 0)
                {
                    mockOrderMap[key]->OrderStatus = define::FILLED;
                    mockOrderMap[key]->ExecuteQuantity = tradeQuantity;
                    mockOrderMap[key]->ExecutePrice = tradePrice;
                    //剩余数量清零
                    mockOrderMap[key]->Quantity = 0;
                }
                if (tradeQuantity <= quantity && tradeQuantity != 0)
                {
                    mockOrderMap[key]->OrderStatus = define::PARTIALLY_FILLED;
                    mockOrderMap[key]->ExecuteQuantity = quantity;
                    mockOrderMap[key]->ExecutePrice = tradePrice;
                    mockOrderMap[key]->Quantity -= tradeQuantity;
                }
            }
            int err;
            triangular.baseOrderHandler(*mockOrderMap[key], err);
        }
        triangular.setOrderMap(mockOrderMap);
    }

    void MakerMock::initMock(){

    };

}

