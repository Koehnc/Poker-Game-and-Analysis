#ifndef POKER_H
#define POKER_H

#include "game/Player.h"
#include "core/Deck.h"
#include "core/Card.h"
#include "strategies/IStrategy.h"
#include <vector>
#include <memory>
#include <random>

namespace poker {
    class Poker {
        public:
            static const unsigned int BigBlind = 2;
            static const unsigned int SmallBlind = 1;

            Poker(int numPlayers, int startingMoney, bool quiet=false);
            Poker(std::vector<std::unique_ptr<IStrategy>> strategies, int startingMoney, bool quiet=false);
            void restartGame();
            int simHand(); //#ThisIsBasicallyTheRunner

            std::vector<ActionRecord>* betsIn(int startingPosition, poker::Round round, std::vector<ActionRecord> *streetHistory);
            void collectStats(std::vector<ActionRecord>* handHistory);
            void collectPreflopStats(std::vector<ActionRecord> *handHistory); // Could be private
            void collectFlopStats(std::vector<ActionRecord> *handHistory); // Could be private
            void collectTurnStats(std::vector<ActionRecord> *handHistory); // Could be private
            void collectRiverStats(std::vector<ActionRecord> *handHistory); // Could be private

            void deal();
            void flop();
            void turn();
            void river();

            bool onlyOnePerson();
            int getWinner();
            double getMonteEquity(unsigned int numSims, unsigned int playerIndex);
            int getPlayerChips(int playerIndex) const;

            void printTable();
            void printPlayers();
            void savePlayerStats();

        private:
            int mNumPlayers;
            std::vector<Player> mPlayers;
            std::vector<bool> mActivePlayers;
            std::vector<int> mBetsFromPlayers;
            std::unique_ptr<Deck> mDeck;
            bool mQuiet;

            int mPot;
            int mMinCall;
            int mDealerIndex;
            int mCurrentBetter;
            int mPreviousStreetAggressor;
            std::vector<Card> mBoard;
    };
}

#endif