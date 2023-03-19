#include "define/define.h"
#include "node.h"

namespace Pathfinder
{
    Node::Node(int base, int quote) : baseIndex(base), quoteIndex(quote)
    {

    }

    double Node::GetOriginPrice(int fromIndex, int toIndex)
    {
        if (fromIndex == this->baseIndex && toIndex == this->quoteIndex)
        {
            if (this->sellDepth.size() == 0)
                return 0;
            return this->sellDepth[0].Price;
        }

        if (fromIndex == this->quoteIndex && toIndex == this->baseIndex)
        {
            if (this->buyDepth.size() == 0)
                return 0;
            return this->buyDepth[0].Price;
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

    double Node::GetParsePathPrice(double price, int fromIndex, int toIndex){
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
        if (this->sellDepth.size() == 0 || this->buyDepth.size() == 0) {
            return 0;
        }

        if (fromIndex == this->baseIndex && toIndex == this->quoteIndex) {
            return this->sellDepth[0].Quantity;
        }

        if (fromIndex == this->quoteIndex && toIndex == this->baseIndex) {
            return this->buyDepth[0].Quantity;
        }

        return 0;
    }

    double Node::UpdateSell(vector<WebsocketWrapper::DepthItem> depth)
    {
        this->sellDepth = depth;
    }

    double Node::UpdateBuy(vector<WebsocketWrapper::DepthItem> depth)
    {
        this->buyDepth = depth;
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
            this->sellDepth[0].Price = price;
        }

        if (fromIndex == this->quoteIndex && toIndex == this->baseIndex)
        {
            this->buyDepth[0].Price = price;
        }
    }

    vector<WebsocketWrapper::DepthItem> Node::getDepth(int fromIndex, int toIndex){
        if (fromIndex == this->baseIndex && toIndex == this->quoteIndex)
        {
            return sellDepth;
        }

        if (fromIndex == this->quoteIndex && toIndex == this->baseIndex)
        {
            return buyDepth;
        }
    }

    vector<int> Node::mockGetIndexs(){
        vector<int> indexs;
        indexs[0] = baseIndex;
        indexs[1] = quoteIndex;
        return indexs;
    }
}
