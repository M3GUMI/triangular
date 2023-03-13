#include "define/define.h"
#include "node.h"
#include "lib/arbitrage/triangular/maker_mock.h"

namespace Pathfinder
{
    Node::Node(int base, int quote) : baseIndex(base), quoteIndex(quote)
    {

    }

    double Node::GetOriginPrice(int fromIndex, int toIndex)
    {
        if (fromIndex == this->baseIndex && toIndex == this->quoteIndex)
        {
            return this->sellPrice;
        }

        if (fromIndex == this->quoteIndex && toIndex == this->baseIndex)
        {
            return this->buyPrice;
        }

        return 0;
    }

    double Node::GetParsePrice(int fromIndex, int toIndex)
    {
        auto price = GetOriginPrice(fromIndex, toIndex);
        if (price == 0) {
            return 0;
        }

        if (fromIndex == this->quoteIndex && toIndex == this->baseIndex)
        {
            return 1/price;
        } else {
            return price;
        }
    }

    double Node::GetQuantity(int fromIndex, int toIndex)
    {
        if (fromIndex == this->baseIndex && toIndex == this->quoteIndex) {
            return this->sellQuantity;
        }

        if (fromIndex == this->quoteIndex && toIndex == this->baseIndex) {
            return this->buyQuantity;
        }

        return 0;
    }

    double Node::UpdateSell(double price, double quantity)
    {
        this->sellPrice = price;
        this->sellQuantity = quantity;
    }

    double Node::UpdateBuy(double price, double quantity)
    {
        this->buyPrice = price;
        this->buyQuantity = quantity;
    }

    TransactionPathItem Node::Format(conf::Step& step, map<int, string>& indexToToken, int from, int to) {
        TransactionPathItem item{};
        item.BaseToken = indexToToken[baseIndex];
        item.QuoteToken = indexToToken[quoteIndex];
        item.OrderType = step.OrderType;
        item.TimeInForce = step.TimeInForce;
        item.Price = GetOriginPrice(from, to);
        item.Quantity = GetQuantity(from, to);

        if (from == baseIndex && to == quoteIndex) {
            item.Side = define::SELL;
        } else {
            item.Side = define::BUY;
        }

        return item;
    };

    void Node::mockSetOriginPrice(int fromIndex, int toIndex, double price){

        if (fromIndex == this->baseIndex && toIndex == this->quoteIndex)
        {
            this->sellPrice = price;
        }

        if (fromIndex == this->quoteIndex && toIndex == this->baseIndex)
        {
            this->buyPrice = price;
        }
    }
    vector<int> Node::mockGetIndexs(){
        vector<int> indexs;
        indexs[0] = baseIndex;
        indexs[1] = quoteIndex;
        return indexs;
    }
}
