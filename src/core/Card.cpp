#include "core/Card.h"

#include <string>
#include <sstream>

namespace poker 
{
    Card::Card(int rank, int suit) :
    mRank(rank),
    mSuit(suit)
    {}

    int Card::getRank() const
    {
        return mRank;
    }

    int Card::getSuit() const
    {
        return mSuit;
    }
    
    std::string Card::toString() const
    {
        std::stringstream ss;
        switch (mRank) 
        {
            case 11:
                ss << "J";
                break;
            case 12:
                ss << "Q";
                break;
            case 13:
                ss << "K";
                break;
            case 14:
                ss << "A";
                break;
            default:
                ss << std::to_string(mRank);
                break;
        }

        switch (mSuit) 
        {
            case 0:
                ss << "c";
                break;
            case 1:
                ss << "d";
                break;
            case 2:
                ss << "h";
                break;
            case 3:
                ss << "s";
                break;
            default:
                ss << "Mis-Suit";
                break;
        }
        return ss.str();
    }
}