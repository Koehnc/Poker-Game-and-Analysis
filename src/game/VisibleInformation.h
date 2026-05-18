#ifndef VISIBLE_INFORMATION_H
#define VISIBLE_INFORMATION_H

#include "core/Card.h"

#include <vector>

namespace poker
{
    enum class ActionType { Fold, Call, Raise, Check };
    enum class Round { Preflop, Flop, Turn, River };

    struct Stat {
        int opportunities = 0;
        int successes = 0;

        double pct() const {
            return opportunities > 0 ? (double)successes / opportunities : 0.0;
        }
    };

    struct PreflopStats
    {
        Stat VPIP; //Voluntarily put money in pot
        Stat raise;
        Stat call;
        Stat threeBet;
        Stat foldToThreeBet;
    };

    struct PostFlopStats
    {
        // When they are the aggressor
        Stat cBetFlop;
        Stat cBetTurn;
        Stat cBetRiver;

        // When facing aggression
        Stat foldToCBetFlop;
        Stat foldToCBetTurn;
        Stat foldToCBetRiver;

        // General aggression
        Stat betOrRaise;   // total aggressive actions
        Stat call;         // passive actions
        Stat checkRaise;    // trappy action, not implemented, hasBet or maybe some tricky numBet = 0 and index shenanigans
    };

    struct PlayerStats 
    {
        int playerId;
        PreflopStats preflopStats;
        PostFlopStats postFlopStats;
    };

    struct Action 
    {
        ActionType type;
        int amount; // only used for Bet/Raise
    };

    struct ActionRecord 
    {
        int playerId;
        Round round;         // preflop, flop, turn, river
        ActionType action;  // Fold, Call, Raise, Check
        int amount;        // for bets/raises

        int pot;
        int lastAggressor;
        int lastStreetAggressor;
    };

    struct GameStateView 
    {
        std::vector<Card> board;
        int pot;
        int minToCall;
        int playerPosition;
        int currentBetterInd;   // -1, no bet present
        PlayerStats currentBetterStats;
        int playerStack;
        std::vector<Card> playerHand;
        Round round;

        int numRemainingPlayers;    // Eventually this could be gotten rid of and the strategies can parse the actionHistory
        std::vector<ActionRecord> actionHistory;
    };
}

#endif