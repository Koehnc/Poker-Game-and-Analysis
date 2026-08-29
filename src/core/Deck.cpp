#include "core/Deck.h"

#include <random>
#include <algorithm>
#include <iostream>

namespace poker 
{
    Deck::Deck()
    {
        for (int r = 2; r < 15; ++r) 
        {
            for (int s = 0; s < 4; ++s) 
            {
                mCards.push_back(Card(r, s));
            }
        }

        shuffle();
    }

    void Deck::reset()
    {
        mCards.clear();

        for (int r = 2; r < 15; ++r) 
        {
            for (int s = 0; s < 4; ++s) 
            {
                mCards.push_back(Card(r, s));
            }
        }

        shuffle();
    }

    void Deck::shuffle()
    {
        std::random_device rd;
        unsigned int seed = rd();
        std::mt19937 rng(rd());

        // std::cout << "Using random seed: " << seed << std::endl;

        std::shuffle(mCards.begin(), mCards.end(), rng);
    }

    Card Deck::draw()
    {
        Card lastCard = mCards.back();
        mCards.pop_back();
        return lastCard;
    }

    void Deck::addBack(Card card)
    {
        mCards.push_back(card);
    }

    void Deck::remove(Card card)
    {
        // Card::operator== only compares rank (intentional — Hand's straight
        // detection relies on that to collapse same-rank duplicates), so an
        // exact match here needs rank and suit checked explicitly.
        auto cardInd = std::find_if(mCards.begin(), mCards.end(), [&card](const Card& c) {
            return c.getRank() == card.getRank() && c.getSuit() == card.getSuit();
        });
        if (cardInd != mCards.end()) mCards.erase(cardInd);
    }

    int Deck::getSize()
    {
        return mCards.size();
    }
}