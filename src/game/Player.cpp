#include "game/Player.h"
#include "game/VisibleInformation.h"
#include "strategies/RandomStrategy.h"

#include <random>
#include <memory>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>

namespace poker
{
    Player::Player(int startingChips, std::unique_ptr<IStrategy> strat):
    mChips(startingChips),
    mStrategy(std::move(strat)),
    mPlayerStats()
    {
    }

    std::vector<Card> Player::getHand()
    {
        return mHand;
    }

    int Player::getChips() const
    {
        return mChips;
    }
    
    void Player::pickUp(Card card)
    {
        mHand.push_back(card);
    }

    void Player::emptyHand()
    {
        mHand.clear();
    }

    void Player::recordHand(bool played)
    {
        Card card1 = *std::max_element(mHand.begin(), mHand.end());
        Card card2 = *std::min_element(mHand.begin(), mHand.end());

        int row = 0;
        int col = 0;
        if (card1.getSuit() == card2.getSuit())
        {
            row = 14 - card1.getRank();
            col = 14 - card2.getRank();
        }
        else
        {
            row = 14 - card2.getRank();
            col = 14 - card1.getRank();
        }

        if (played) 
        {
            mHandStats[row][col].played++;
            mHandStats[row][col].dealt++;
        }
        else mHandStats[row][col].dealt++;
    }

    void Player::payout(int wonSum)
    {
        mChips += wonSum;
    }

    Action Player::getAction(GameStateView state)
    {
        state.playerStack = mChips;
        Action action = mStrategy->decide(state);
        mChips -= action.amount;
        return action;
    }

    int Player::bet(int betSize)
    {
        if (betSize <= mChips)
        {
            mChips -= betSize;
            return betSize;
        }
        else 
        {
            // All in
            betSize = mChips;
            mChips = 0;
            return betSize;
        }
    }

    void Player::saveToFile(std::string fileName)
    {
        std::ofstream outFile(fileName);

        outFile << "=====================" << std::endl;
        outFile << "Player Id: " << mPlayerStats.playerId << std::endl;
        outFile << "Player strategy: " << typeid(mStrategy).name() << std::endl;
        outFile << "=====================" << std::endl;
        outFile << std::endl;
        outFile << "------------PREFLOP STATS------------" << std::endl;
        outFile << "VPIP: " << mPlayerStats.preflopStats.VPIP.successes << "/" << mPlayerStats.preflopStats.VPIP.opportunities << std::endl;
        outFile << "call: " << mPlayerStats.preflopStats.call.successes << "/" << mPlayerStats.preflopStats.call.opportunities << std::endl;
        outFile << "raise: " << mPlayerStats.preflopStats.raise.successes << "/" << mPlayerStats.preflopStats.raise.opportunities << std::endl;
        outFile << "threeBet: " << mPlayerStats.preflopStats.threeBet.successes << "/" << mPlayerStats.preflopStats.threeBet.opportunities << std::endl;
        outFile << "foldToThreeBet: " << mPlayerStats.preflopStats.foldToThreeBet.successes << "/" << mPlayerStats.preflopStats.foldToThreeBet.opportunities << std::endl;
        outFile << std::endl;
        outFile << "------------POSTFLOP STATS------------" << std::endl;
        outFile << "call: " << mPlayerStats.postFlopStats.call.successes << "/" << mPlayerStats.postFlopStats.call.opportunities << std::endl;
        outFile << "betOrRaise: " << mPlayerStats.postFlopStats.betOrRaise.successes << "/" << mPlayerStats.postFlopStats.betOrRaise.opportunities << std::endl;
        outFile << "cBetFlop: " << mPlayerStats.postFlopStats.cBetFlop.successes << "/" << mPlayerStats.postFlopStats.cBetFlop.opportunities << std::endl;
        outFile << "foldToCBetFlop: " << mPlayerStats.postFlopStats.foldToCBetFlop.successes << "/" << mPlayerStats.postFlopStats.foldToCBetFlop.opportunities << std::endl;
        outFile << "cBetTurn: " << mPlayerStats.postFlopStats.cBetTurn.successes << "/" << mPlayerStats.postFlopStats.cBetTurn.opportunities << std::endl;
        outFile << "foldToCBetTurn: " << mPlayerStats.postFlopStats.foldToCBetTurn.successes << "/" << mPlayerStats.postFlopStats.foldToCBetTurn.opportunities << std::endl;
        outFile << "cBetRiver: " << mPlayerStats.postFlopStats.cBetRiver.successes << "/" << mPlayerStats.postFlopStats.cBetRiver.opportunities << std::endl;
        outFile << "foldToCBetRiver: " << mPlayerStats.postFlopStats.foldToCBetRiver.successes << "/" << mPlayerStats.postFlopStats.foldToCBetRiver.opportunities << std::endl;
        outFile << std::endl;
        outFile << std::endl;

        outFile << "=====================" << std::endl;
        outFile << "Range Played (Preflop)" << std::endl;
        outFile << "=====================" << std::endl;
        std::vector<std::string> indices = {"A", "K", "Q", "J", "10", "9", "8", "7", "6", "5", "4", "3", "2"};
        // Top row
        std::string gap = "   ";
        outFile << gap << " ";
        for (std::string col : indices)
        {
            outFile << "|" << col << "|" << gap;
        }
        outFile << std::endl;

        for (int r = 0; r < 13; ++r)
        {
            outFile << "|" << indices[r] << "| ";
            if (indices[r] != "10") outFile << " ";
            for (int c = 0; c < 13; ++c)
            {
                outFile << mHandStats[r][c].played << "/" << mHandStats[r][c].dealt << gap;
            }
            outFile << std::endl;
        }

        outFile.close();
    }
}