#ifndef PLAYER_H
#define PLAYER_H

#include <vector>
#include <memory>

#include "core/Card.h"
#include "game/VisibleInformation.h"
#include "strategies/IStrategy.h"

namespace poker
{
    struct HandStats
    {
        int dealt = 0;
        int played = 0;
    };

    class Player
    {
        public:
            Player(int startingChips, std::unique_ptr<IStrategy> strat);
            std::vector<Card> getHand();
            int getChips() const;
            void pickUp(Card card);
            void emptyHand();
            void recordHand(bool played = false);
            void payout(int wonSum);

            Action getAction(GameStateView state);
            int bet(int betSize);

            PlayerStats mPlayerStats;
            void saveToFile(std::string fileName);

        private:
            std::vector<Card> mHand;
            int mChips;
            std::unique_ptr<IStrategy> mStrategy;
            HandStats mHandStats[13][13];
    };
}

#endif