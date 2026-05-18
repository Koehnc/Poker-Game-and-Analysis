
#include "core/Hand.h"

#include <vector>
#include <cmath>
#include <algorithm>
#include <iostream>

namespace poker
{    
    Hand::Hand(std::vector<Card> cards1, std::vector<Card> cards2, std::vector<Card> cards3):
    mCards()
    {
        mCards.insert(mCards.end(), cards1.begin(), cards1.end());
        mCards.insert(mCards.end(), cards2.begin(), cards2.end());
        mCards.insert(mCards.end(), cards3.begin(), cards3.end());
        std::sort(mCards.begin(), mCards.end());
        mHandValue = calcHandValue();
    }

    Hand::Hand(std::vector<Card> cards1, std::vector<Card> cards2):
    mCards()
    {
        mCards.insert(mCards.end(), cards1.begin(), cards1.end());
        mCards.insert(mCards.end(), cards2.begin(), cards2.end());
        std::sort(mCards.begin(), mCards.end());
        mHandValue = calcHandValue();
    }

    Hand::Hand(std::vector<Card> cards):
    mCards()
    {
        mCards.insert(mCards.end(), cards.begin(), cards.end());
        std::sort(mCards.begin(), mCards.end());
        mHandValue = calcHandValue();
    }
    
    long long Hand::getHandValue()
    {
        return mHandValue;
    }

    long long Hand::calcHandValue()
    {
        long long handValue = 0;
        
        long long straightFlush = getStraightFlush(mCards);
        if (straightFlush != 0) handValue = straightFlush * 1000000LL + 80000000000;

        long long quads = getFourPair(mCards);
        if (handValue == 0 && quads != 0) handValue = quads * 1000000LL + 70000000000LL;

        long long fullHouse = getFullHouse(mCards);
        if (handValue == 0 && fullHouse != 0) handValue = fullHouse * 1000000LL + 60000000000LL;

        long long flush = getFlush(mCards);
        if (handValue == 0 && flush != 0) handValue = flush * 1000000LL + 50000000000LL;

        long long straight = getStraight(mCards);
        if (handValue == 0 && straight != 0) handValue = straight * 1000000LL + 40000000000LL;

        long long threePair = getThreePair(mCards);
        if (handValue == 0 && threePair != 0)
        { 
            handValue = threePair * 1000000LL + 30000000000LL;
            handValue += getHighCardValue(2, {(int)threePair}, mCards);
        }

        long long twoPair = getTwoPair(mCards);
        if (handValue == 0 && twoPair != 0) 
        {
            handValue = twoPair * 1000000LL + 20000000000LL;
            handValue += getHighCardValue(2, {(int)twoPair % 100, (int)twoPair / 100}, mCards);
        }

        long long pair = getPair(mCards);
        if (handValue == 0 && pair != 0) 
        {
            handValue = pair * 1000000LL + 10000000000LL;
            handValue += getHighCardValue(3, {(int)pair}, mCards);
        }

        if (handValue == 0) handValue = getHighCardValue(5, {}, mCards);

        return handValue;
    }

    long long Hand::getHighCardValue(int numHighCards, std::vector<int> ranksToSkip, std::vector<Card> cards)
    {
        long long value = 0;
        for (int j = (int)cards.size() - 1; j >= 0; --j)
        {
            if (numHighCards == 0)
            {
                break; // the for loop
            }
            if (std::count(ranksToSkip.begin(), ranksToSkip.end(), cards[j].getRank()) == 0)
            {
                value += std::pow(10, (numHighCards - 1) * 2) * cards[j].getRank();
                --numHighCards;
            }
        }
        return value;
    }

    int Hand::getPair(std::vector<Card> cards)
    {
        for (int i = 0 ; i < (int)cards.size() - 1; ++i)
        {
            if (cards[i].getRank() == cards[i+1].getRank())
            {
                return cards[i].getRank();
            }
        }
        return 0;
    }

    // Fucks up when there's 3 pairs (Takes the lowest two)
    int Hand::getTwoPair(std::vector<Card> cards)
    {
        for (int i = 0 ; i < (int)cards.size() - 1; ++i)
        {
            if (cards[i].getRank() == cards[i+1].getRank())
            {
                std::vector<Card> newCards(cards.begin() + i + 1, cards.end());
                int ret = getPair(newCards);
                if (ret != 0)
                {
                    return ret * 100 + cards[i].getRank();
                }
            }
        }
        return 0;
    }
    
    int Hand::getThreePair(std::vector<Card> cards)
    {
        for (int i = 0 ; i < (int)cards.size() - 2; ++i)
        {
            if (cards[i].getRank() == cards[i+1].getRank() && cards[i].getRank() == cards[i+2].getRank())
            {
                return cards[i].getRank();
            }
        }
        return 0;
    }
    
    int Hand::getStraight(std::vector<Card> cards)
    {
        //Check for Ace, add to beginning for low AND HIGH straights
        if (cards.back().getRank() == 14)
        {
            cards.insert(cards.begin(), cards.back());
        }
        
        // delete repeats/pairs
        cards.erase(std::unique(cards.begin(), cards.end()), cards.end());

        for (int i = 0 ; i < (int)cards.size() - 4; ++i)
        {
            // std::cout << i << std::endl;
            // Mod % 14 changes Ace value to one for straight
            if (cards[i].getRank() % 14 + 1 == cards[i+1].getRank() &&
            cards[i].getRank() + 2 == cards[i+2].getRank() &&
            cards[i].getRank() + 3 == cards[i+3].getRank() &&
            cards[i].getRank() + 4 == cards[i+4].getRank())
            {
                return cards[i].getRank() + 4;
            }
        }
        return 0;
    }

    int Hand::getFlush(std::vector<Card> cards)
    {
        int numc = 0;
        int numd = 0;
        int numh = 0;
        int nums = 0;

        int highestC = 0;
        int highestD = 0;
        int highestH = 0;
        int highestS = 0;

        for (Card card : cards)
        {
            switch (card.getSuit())
            {
                case 0:
                    numc++;
                    highestC = card.getRank();
                    break;
                case 1:
                    numd++;
                    highestD = card.getRank();
                    break;
                case 2:
                    numh++;
                    highestH = card.getRank();
                    break;
                case 3:
                    nums++;
                    highestS = card.getRank();
                    break;
                default:
                    break;
            }
        }
        if (numc >= 5)
        {
            return highestC;
        }
        if (numd >= 5)
        {
            return highestD;
        }
        if (numh >= 5)
        {
            return highestH;
        }
        if (nums >= 5)
        {
            return highestS;
        }
        return 0;
    }

    int Hand::getFullHouse(std::vector<Card> cards)
    {
        for (int i = 0 ; i < (int)cards.size() - 1; ++i)
        {
            // if a pair and no trips, is there a remaining trips
            if (cards[i].getRank() == cards[i+1].getRank() && i+2 < cards.size() && cards[i].getRank() != cards[i+2].getRank())
            {
                std::vector<Card> newCards(cards.begin() + i + 1, cards.end());
                int ret = getThreePair(newCards);
                if (ret != 0)
                {
                    return ret * 100 + cards[i].getRank();
                }
            }
            
            // If there's trips, is there a remaining pair
            if (cards[i].getRank() == cards[i+1].getRank() && i+2 < cards.size() && cards[i].getRank() == cards[i+2].getRank())
            {
                std::vector<Card> newCards(cards.begin() + i + 2, cards.end());
                int ret = getPair(newCards);
                if (ret != 0)
                {
                    return cards[i].getRank() * 100 + ret;
                }
            }
        }
        return 0;
    }
    
    int Hand::getFourPair(std::vector<Card> cards)
    {
        for (int i = 0 ; i < (int)cards.size() - 3; ++i)
        {
            if (cards[i].getRank() == cards[i+1].getRank() &&
            cards[i].getRank() == cards[i+2].getRank() &&
            cards[i].getRank() == cards[i+3].getRank())
            {
                return cards[i].getRank();
            }
        }
        return 0;
    }
    
    int Hand::getStraightFlush(std::vector<Card> cards)
    {
        int flush = getFlush(cards);
        int straight = getStraight(cards);
        if (flush != 0 && straight != 0)
        {   
            //Check for Ace, add to beginning for low AND HIGH straights
            if (cards.back().getRank() == 14)
            {
                cards.insert(cards.begin(), cards.back());
            }
            
            // delete repeats/pairs
            cards.erase(std::unique(cards.begin(), cards.end()), cards.end());

            for (int i = 0 ; i < (int)cards.size() - 4; ++i)
            {
                // Mod % 14 changes Ace value to one for straight
                if (cards[i].getRank() % 14 + 1 == cards[i+1].getRank() && cards[i].getSuit() == cards[i+1].getSuit() &&
                cards[i].getRank() + 2 == cards[i+2].getRank() && cards[i].getSuit() == cards[i+2].getSuit() &&
                cards[i].getRank() + 3 == cards[i+3].getRank() && cards[i].getSuit() == cards[i+3].getSuit() &&
                cards[i].getRank() + 4 == cards[i+4].getRank() && cards[i].getSuit() == cards[i+4].getSuit())
                {
                    return cards[i+4].getRank();
                }
            }
        }
        return 0;
    }
}
