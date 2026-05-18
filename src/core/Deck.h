#ifndef DECK_H
#define DECK_H

#include "core/Card.h"
#include <vector>

namespace poker {
    class Deck {
        public:
            Deck();
            void reset();
            void shuffle();
            Card draw();
            void addBack(Card card);
            void remove(Card card);
            int getSize();

        private:
            std::vector<Card> mCards;
    };
}

#endif