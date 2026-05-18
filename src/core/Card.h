#ifndef CARD_H
#define CARD_H

#include <string>

namespace poker {
    class Card {
        public:
            Card(int rank, int suit);

            bool operator>(const Card other) const
            {
                return mRank > other.getRank();
            }

            bool operator<(const Card other) const
            {
                return mRank < other.getRank();
            }

            bool operator==(const Card other) const
            {
                return mRank == other.getRank();
            }

            std::string toString() const;
            int getRank() const;
            int getSuit() const;

        private:
            // Rank is the number 0-12 Ace - King respectively
            int mRank;

            // Suit is the suit (Alphabetical): 0: Club 1: Diamond 2: Heart 3: Spade
            int mSuit;

    };
}

#endif