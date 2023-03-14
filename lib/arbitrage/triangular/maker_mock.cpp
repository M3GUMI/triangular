#include "iostream"
#include <sstream>
#include "maker_mock.h"
#include "utils/utils.h"

using namespace boost::asio;
using namespace std;
using namespace Arbitrage;

namespace MakerMock{

    MakerMock::MakerMock(
            websocketpp::lib::asio::io_service& ioService,
            Pathfinder::Pathfinder &pathfinder,
            Arbitrage::TriangularArbitrage triangular)
            : triangular(triangular),pathfinder(pathfinder), ioService(ioService)
    {}


    MakerMock::~MakerMock()  = default;

    /**
     *  TODO 在node处。价格变动调用该handler进行mock测试
     * */
    void MakerMock::tradeNodeHandler(map<u_int64_t, Pathfinder::Node*> tradeNodeMap){
        mockTradeNodeMap = tradeNodeMap;
        mockOrderMap = triangular.getOrderMap();
        mockTrader(mockTradeNodeMap, mockOrderMap);
    };

    /**
     *  TODO 逻辑与baseOrderHandler合并简化
     * */
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
                    mockOrderMap[key]->UpdateTime = GetNowTime();
                }
                if (tradeQuantity <= quantity && tradeQuantity != 0)
                {
                    mockOrderMap[key]->OrderStatus = define::PARTIALLY_FILLED;
                    mockOrderMap[key]->ExecuteQuantity = quantity;
                    mockOrderMap[key]->ExecutePrice = tradePrice;
                    mockOrderMap[key]->Quantity -= tradeQuantity;
                    mockOrderMap[key]->UpdateTime = GetNowTime();
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
                    mockOrderMap[key]->UpdateTime = GetNowTime();
                }
                if (tradeQuantity <= quantity && tradeQuantity != 0)
                {
                    mockOrderMap[key]->OrderStatus = define::PARTIALLY_FILLED;
                    mockOrderMap[key]->ExecuteQuantity = quantity;
                    mockOrderMap[key]->ExecutePrice = tradePrice;
                    mockOrderMap[key]->Quantity -= tradeQuantity;
                    mockOrderMap[key]->UpdateTime = GetNowTime();
                }
            }
            int err;
            triangular.baseOrderHandler(*mockOrderMap[key], err);
        }
        triangular.setOrderMap(mockOrderMap);
    }
    /**
     *  TODO 在node的mockSetPrice方法内调用tradeNodeHandler来手动将新的价格传给handler
     * */
    void MakerMock::priceController(map<u_int64_t, Pathfinder::Node*> tradeNodeMap)
    {
        if (EnablePriceController = true)
        {
            for (auto it = tradeNodeMap.begin(); it != tradeNodeMap.end(); it++)
            {
                int key = it->first;
                vector<int> indexs = tradeNodeMap[key]->mockGetIndexs();
                int ram = rand() % 4 + 1;
                double price = 0;
                switch (ram)
                {
                    case 1:
                        price = tradeNodeMap[key]->GetOriginPrice(indexs[0], indexs[1]) * (1 + upDownRaise);
                        tradeNodeMap[key]->mockSetOriginPrice(indexs[0], indexs[1], price);
                        break;
                    case 2:
                        price = tradeNodeMap[key]->GetOriginPrice(indexs[0], indexs[1]) * (1 - upDownRaise);
                        tradeNodeMap[key]->mockSetOriginPrice(indexs[0], indexs[1], price);
                        break;
                    case 3:
                        price = tradeNodeMap[key]->GetOriginPrice(indexs[1], indexs[0]) * (1 + upDownRaise);
                        tradeNodeMap[key]->mockSetOriginPrice(indexs[1], indexs[0], price);
                        break;
                    case 4:
                        price = tradeNodeMap[key]->GetOriginPrice(indexs[1], indexs[0]) * (1 - upDownRaise);
                        tradeNodeMap[key]->mockSetOriginPrice(indexs[1], indexs[0], price);
                        break;
                }
            }
            tradeNodeHandler(tradeNodeMap);
            priceControlTimer = std::make_shared<websocketpp::lib::asio::steady_timer>(ioService,
                                                                                       websocketpp::lib::asio::milliseconds(
                                                                                               30000));
            priceControlTimer->async_wait(bind(&MakerMock::priceController, this, tradeNodeMap));
        }
    }
}

