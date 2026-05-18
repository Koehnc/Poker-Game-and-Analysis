#ifndef HAND_H
#define HAND_H

#include "core/Card.h"

#include <vector>

namespace poker
{
    class Hand
    {
        public:
            Hand(std::vector<Card> cards1, std::vector<Card> cards2, std::vector<Card> cards3);
            Hand(std::vector<Card> cards1, std::vector<Card> cards2);
            Hand(std::vector<Card> cards);

            long long getHandValue();


        private:
            long long calcHandValue();
            
            long long getHighCardValue(int numHighCards, std::vector<int> ranksToSkip, std::vector<Card> cards);
            int getPair(std::vector<Card> cards);
            int getTwoPair(std::vector<Card> cards);
            int getThreePair(std::vector<Card> cards);
            int getStraight(std::vector<Card> cards);
            int getFlush(std::vector<Card> cards);
            int getFullHouse(std::vector<Card> cards);
            int getFourPair(std::vector<Card> cards);
            int getStraightFlush(std::vector<Card> cards);

            std::vector<Card> mCards;

            // concatted # for hand type, ## ## for up to two ranks or high cards, ## ## ## for high card
            long long mHandValue;
    };
}

#endif