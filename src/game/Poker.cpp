#include "game/Poker.h"
#include "game/Player.h"
#include "core/Hand.h"
#include "game/VisibleInformation.h"
#include "strategies/RandomStrategy.h"
#include "strategies/MonteCarloStrategyV1.h"
#include "evaluators/MonteCarloEvaluator.h"

#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <memory>

namespace poker {
    Poker::Poker(int numPlayers, int startingMoney, bool quiet):
    mNumPlayers(numPlayers),
    mBoard(),
    mPlayers(),
    mActivePlayers(),
    mBetsFromPlayers(),
    mDealerIndex(0),
    mCurrentBetter(-1),
    mPreviousStreetAggressor(-1),
    mPot(0),
    mMinCall(0),
    mDeck(std::make_unique<Deck>()),
    mQuiet(quiet)
    {
        // auto monteCarloEval = std::make_unique<MonteCarloEvaluator>();
        // auto monteStrategy = std::make_unique<MonteCarloStrategyV1>(std::move(monteCarloEval));
        // mPlayers.emplace_back(startingMoney, std::move(monteStrategy));
        // mActivePlayers.push_back(true);
        // mBetsFromPlayers.push_back(0);
        // mPlayers[0].mPlayerStats.playerId = 0;

        for (int i = 0; i < numPlayers; ++i)
        {
            // mPlayers.emplace_back(startingMoney, std::make_unique<RandomStrategy>());
            auto monteCarloEval = std::make_unique<MonteCarloEvaluator>();
            auto monteStrategy = std::make_unique<MonteCarloStrategyV1>(std::move(monteCarloEval));
            mPlayers.emplace_back(startingMoney, std::move(monteStrategy));
            mActivePlayers.push_back(true);
            mBetsFromPlayers.push_back(0);
            mPlayers[i].mPlayerStats.playerId = i;
        }
    }

    Poker::Poker(std::vector<std::unique_ptr<IStrategy>> strategies, int startingMoney, bool quiet) :
    mNumPlayers((int)strategies.size()),
    mBoard(),
    mPlayers(),
    mActivePlayers(),
    mBetsFromPlayers(),
    mDealerIndex(0),
    mCurrentBetter(-1),
    mPreviousStreetAggressor(-1),
    mPot(0),
    mMinCall(0),
    mDeck(std::make_unique<Deck>()),
    mQuiet(quiet)
    {
        for (int i = 0; i < (int)strategies.size(); ++i)
        {
            mPlayers.emplace_back(startingMoney, std::move(strategies[i]));
            mActivePlayers.push_back(true);
            mBetsFromPlayers.push_back(0);
            mPlayers[i].mPlayerStats.playerId = i;
        }
    }

    int Poker::getPlayerChips(int playerIndex) const
    {
        return mPlayers[playerIndex].getChips();
    }

    void Poker::setStepCallback(StepCallback cb)
    {
        mStepCallback = std::move(cb);
    }

    void Poker::emitStep(StepKind kind, Round round, const ActionRecord* action)
    {
        if (!mStepCallback) return;
        HandStep step;
        step.kind = kind;
        step.round = round;
        step.board = mBoard;
        step.pot = mPot;
        step.dealerIndex = mDealerIndex;
        if (action) step.action = *action;
        mStepCallback(step);
    }

    void Poker::restartGame()
    {
        mBoard.clear();
        mPot = 0;
        mMinCall = BigBlind;
        ++mDealerIndex %= mNumPlayers;
        mCurrentBetter = -1;
        mPreviousStreetAggressor = -1;
        mDeck->reset();
        for (int i = 0; i < mNumPlayers; ++i)
        {
            mPlayers[i].emptyHand();
            mActivePlayers[i] = true;
            mBetsFromPlayers[i] = 0;
        }
        mPot += mPlayers[(mDealerIndex + 1) % mNumPlayers].bet(SmallBlind);
        mBetsFromPlayers[(mDealerIndex + 1) % mNumPlayers] = SmallBlind;
        mPot += mPlayers[(mDealerIndex + 2) % mNumPlayers].bet(BigBlind);
        mBetsFromPlayers[(mDealerIndex + 2) % mNumPlayers] = BigBlind;
    }

    // #ThisIsBasicallyTheRunnerRightNow
    int Poker::simHand()
    {
        restartGame();
        int winner = -1;
        std::vector<ActionRecord> streetHistory;

        deal();
        emitStep(StepKind::HandStarted, poker::Round::Preflop);

        if (!mQuiet) std::cout << "First round bets" << std::endl;
        betsIn(mDealerIndex + 3, poker::Round::Preflop, &streetHistory);
        if (onlyOnePerson())
        {
            winner = getWinner();
            collectStats(&streetHistory);
            mPlayers[winner].payout(mPot);
            // std::cout << "Player " << winner+1 << " won " << mPot << std::endl;
            return winner;
        }

        flop();
        emitStep(StepKind::StreetDealt, poker::Round::Flop);

        if (!mQuiet) std::cout << "Second round bets" << std::endl;
        betsIn(mDealerIndex + 1, poker::Round::Flop, &streetHistory);
        if (onlyOnePerson())
        {
            winner = getWinner();
            collectStats(&streetHistory);
            mPlayers[winner].payout(mPot);
            // std::cout << "Player " << winner+1 << " won " << mPot << std::endl;
            return winner;
        }

        turn();
        emitStep(StepKind::StreetDealt, poker::Round::Turn);

        if (!mQuiet) std::cout << "Third round bets" << std::endl;
        betsIn(mDealerIndex + 1, poker::Round::Turn, &streetHistory);
        if (onlyOnePerson())
        {
            winner = getWinner();
            collectStats(&streetHistory);
            mPlayers[winner].payout(mPot);
            // std::cout << "Player " << winner+1 << " won " << mPot << std::endl;
            return winner;
        }

        river();
        emitStep(StepKind::StreetDealt, poker::Round::River);

        if (!mQuiet) std::cout << "Final round bets" << std::endl;
        betsIn(mDealerIndex + 1, poker::Round::River, &streetHistory);
        winner = getWinner();
        collectStats(&streetHistory);
        if (!mQuiet) printTable();

        mPlayers[winner].payout(mPot);
        // std::cout << "Player " << winner+1 << " won " << mPot << std::endl;
        return winner;
    }

    std::vector<ActionRecord>* Poker::betsIn(int startingPosition, poker::Round round, std::vector<ActionRecord> *streetHistory)
    {
        int numberOfBets = 0;
        int numActivePlayers = mNumPlayers;

        for (int i = startingPosition; i < mPlayers.size() + startingPosition; ++i)
        {
            int j = i % mNumPlayers;
            if (!mActivePlayers[j]) continue; // Folded players get skipped

            GameStateView state;
            state.board = mBoard;
            state.minToCall = mMinCall - mBetsFromPlayers[j];
            state.playerPosition = j;
            state.dealerIndex = mDealerIndex;
            state.currentBetterInd = mCurrentBetter;
            if (mCurrentBetter != -1) state.currentBetterStats = mPlayers[mCurrentBetter].mPlayerStats;
            state.playerHand = mPlayers[j].getHand();
            state.pot = mPot;
            state.numRemainingPlayers = numActivePlayers;
            state.round = round;

            Action action = mPlayers[j].getAction(state);
            int betSize = action.amount;

            streetHistory->emplace_back(ActionRecord{ j, round, action.type, action.amount, mPot, mPreviousStreetAggressor, mCurrentBetter } );
            emitStep(StepKind::PlayerActed, round, &streetHistory->back());

            if (action.type == ActionType::Fold)
            {   
                if (!mQuiet) std::cout << "Player " << j+1 << " Folded" << std::endl;
                mActivePlayers[j] = false;
                --numActivePlayers;
            }
            else if (action.type == ActionType::Call)
            {
                mBetsFromPlayers[j] += betSize;
                mPot += betSize;
                if (!mQuiet) std::cout << "Player " << j+1 << " called " << betSize << " in for: " << mBetsFromPlayers[j] << std::endl;
            }
            else if (action.type == ActionType::Check)
            {
                if (!mQuiet) std::cout << "Player " << j+1 << " checked" << std::endl;
            }
            else //Raise
            {
                mCurrentBetter = j;
                numberOfBets++;

                mBetsFromPlayers[j] += betSize;
                mPot += betSize;
                mMinCall = mBetsFromPlayers[j];
                if (!mQuiet) std::cout << "Player " << j+1 << " raised " << betSize << " in for: " << mBetsFromPlayers[j] << std::endl;

                // Re-iterate through players
                startingPosition = j;
                i = j;
            }
        }

        // Some of this might be redundant with restart
        for (int i = 0; i < mBetsFromPlayers.size(); ++i) mBetsFromPlayers[i] = 0;
        mMinCall = 0;
        mPreviousStreetAggressor = mCurrentBetter;
        return streetHistory;
    }

    void Poker::collectStats(std::vector<ActionRecord>* handHistory)
    {
        // The first n = numberOfPlayers, will get the VPIP once and only once for each player
        for (int i = 0; i < mNumPlayers; ++i)
        {
            mPlayers[i].mPlayerStats.preflopStats.VPIP.opportunities++;

            if (handHistory->at(i).action == ActionType::Call || handHistory->at(i).action == ActionType::Raise)
            {
                mPlayers[handHistory->at(i).playerId].mPlayerStats.preflopStats.VPIP.successes++;
                mPlayers[handHistory->at(i).playerId].recordHand(true);
            }
            else    // Fold and Check
            {
                mPlayers[handHistory->at(i).playerId].recordHand();
            };
        }

        // Check the big blind's VPIP
        bool hasVPIPed = false;
        for (ActionRecord action : *handHistory)
        {
            if (action.playerId == (mDealerIndex + 2) % mNumPlayers)
            {
                if (action.action == ActionType::Fold) break;
                else if (action.action == ActionType::Check) continue;
                else hasVPIPed = true;
            }
        }
        if (hasVPIPed) mPlayers[(mDealerIndex + 2) % mNumPlayers].mPlayerStats.preflopStats.VPIP.successes++;

        collectPreflopStats(handHistory);
        collectFlopStats(handHistory);
        collectTurnStats(handHistory);
        collectRiverStats(handHistory);
    }

    void Poker::collectPreflopStats(std::vector<ActionRecord>* handHistory)
    {
        int numberOfBets = 0;
        for (int i = 0; i < handHistory->size(); ++i)
        {
            if (handHistory->at(i).round != Round::Preflop) continue;
            
            // Every turn except the big blind with no raise has call ops
            if (!(handHistory->at(i).playerId == mDealerIndex + 2 && numberOfBets == 0))
            {
                mPlayers[handHistory->at(i).playerId].mPlayerStats.preflopStats.call.opportunities++;
            }
            // Every turn has the option to raise
            mPlayers[handHistory->at(i).playerId].mPlayerStats.preflopStats.raise.opportunities++;

            // If there's been a bet, the next one is a three bet; ++threeBet ops
            // folding to three bets counts ALL times someone 3 bets, not only the player who initially raised (Ok, not perf.)
            if (numberOfBets == 1)
            {
                mPlayers[handHistory->at(i).playerId].mPlayerStats.preflopStats.threeBet.opportunities++;
                mPlayers[handHistory->at(i).playerId].mPlayerStats.preflopStats.foldToThreeBet.opportunities++;
            }

            // If the action is a call
            if (handHistory->at(i).action == ActionType::Call)
            {
                mPlayers[handHistory->at(i).playerId].mPlayerStats.preflopStats.call.successes++;
            }
            else if (handHistory->at(i).action == ActionType::Raise)
            {
                mPlayers[handHistory->at(i).playerId].mPlayerStats.preflopStats.raise.successes++;
                if (numberOfBets == 1)
                {
                    mPlayers[handHistory->at(i).playerId].mPlayerStats.preflopStats.threeBet.successes++;
                }

                ++numberOfBets;
            }
            else if (handHistory->at(i).action == ActionType::Fold)
            {
                if (numberOfBets == 1)
                {
                    mPlayers[handHistory->at(i).playerId].mPlayerStats.preflopStats.foldToThreeBet.successes++;
                }
            }
        }
    }
    
    void Poker::collectFlopStats(std::vector<ActionRecord>* handHistory)
    {
        int numberOfBets = 0;
        for (int i = 0; i < handHistory->size(); ++i)
        {
            if (handHistory->at(i).round != Round::Flop) continue;
            
            // Every turn has the option to raise
            mPlayers[handHistory->at(i).playerId].mPlayerStats.postFlopStats.betOrRaise.opportunities++;
            // Every turn after a raise can call
            if (numberOfBets > 0) mPlayers[handHistory->at(i).playerId].mPlayerStats.postFlopStats.call.opportunities++;

            // Cbet chance if no one has bet since the previous streetAggressor
            if (handHistory->at(i).lastStreetAggressor == handHistory->at(i).playerId && numberOfBets == 0)
            {
                mPlayers[handHistory->at(i).playerId].mPlayerStats.postFlopStats.cBetFlop.opportunities++;
                if (handHistory->at(i).action == ActionType::Raise)
                {
                    mPlayers[handHistory->at(i).playerId].mPlayerStats.postFlopStats.cBetFlop.successes++; 
                }
            }

            // If Cbet has occurred
            if (handHistory->at(i).lastStreetAggressor == handHistory->at(i).lastAggressor && numberOfBets == 1)
            {
                mPlayers[handHistory->at(i).playerId].mPlayerStats.postFlopStats.foldToCBetFlop.opportunities++;
                if (handHistory->at(i).action == ActionType::Fold)
                {
                    mPlayers[handHistory->at(i).playerId].mPlayerStats.postFlopStats.foldToCBetFlop.successes++;
                }
            }

            if (handHistory->at(i).action == ActionType::Call)
            {
                mPlayers[handHistory->at(i).playerId].mPlayerStats.postFlopStats.call.successes++;
            }
            else if (handHistory->at(i).action == ActionType::Raise)
            {
                mPlayers[handHistory->at(i).playerId].mPlayerStats.postFlopStats.betOrRaise.successes++;
                ++numberOfBets;
            }
        }
    }
    
    void Poker::collectTurnStats(std::vector<ActionRecord>* handHistory)
    {
        int numberOfBets = 0;
        for (int i = 0; i < handHistory->size(); ++i)
        {
            if (handHistory->at(i).round != Round::Turn) continue;
            
            // Every turn has the option to raise
            mPlayers[handHistory->at(i).playerId].mPlayerStats.postFlopStats.betOrRaise.opportunities++;
            // Every turn after a raise can call
            if (numberOfBets > 0) mPlayers[handHistory->at(i).playerId].mPlayerStats.postFlopStats.call.opportunities++;

            // Cbet chance if no one has bet since the previous streetAggressor
            if (handHistory->at(i).lastStreetAggressor == handHistory->at(i).playerId && numberOfBets == 0)
            {
                mPlayers[handHistory->at(i).playerId].mPlayerStats.postFlopStats.cBetTurn.opportunities++;
                if (handHistory->at(i).action == ActionType::Raise)
                {
                    mPlayers[handHistory->at(i).playerId].mPlayerStats.postFlopStats.cBetTurn.successes++; 
                }
            }

            // If Cbet has occurred
            if (handHistory->at(i).lastStreetAggressor == handHistory->at(i).lastAggressor && numberOfBets == 1)
            {
                mPlayers[handHistory->at(i).playerId].mPlayerStats.postFlopStats.foldToCBetTurn.opportunities++;
                if (handHistory->at(i).action == ActionType::Fold)
                {
                    mPlayers[handHistory->at(i).playerId].mPlayerStats.postFlopStats.foldToCBetTurn.successes++;
                }
            }

            if (handHistory->at(i).action == ActionType::Call)
            {
                mPlayers[handHistory->at(i).playerId].mPlayerStats.postFlopStats.call.successes++;
            }
            else if (handHistory->at(i).action == ActionType::Raise)
            {
                mPlayers[handHistory->at(i).playerId].mPlayerStats.postFlopStats.betOrRaise.successes++;
                ++numberOfBets;
            }
        }
    }
    
    void Poker::collectRiverStats(std::vector<ActionRecord>* handHistory)
    {
        int numberOfBets = 0;
        for (int i = 0; i < handHistory->size(); ++i)
        {
            if (handHistory->at(i).round != Round::River) continue;
            
            // Every turn has the option to raise
            mPlayers[handHistory->at(i).playerId].mPlayerStats.postFlopStats.betOrRaise.opportunities++;
            // Every turn after a raise can call
            if (numberOfBets > 0) mPlayers[handHistory->at(i).playerId].mPlayerStats.postFlopStats.call.opportunities++;

            // Cbet chance if no one has bet since the previous streetAggressor
            if (handHistory->at(i).lastStreetAggressor == handHistory->at(i).playerId && numberOfBets == 0)
            {
                mPlayers[handHistory->at(i).playerId].mPlayerStats.postFlopStats.cBetRiver.opportunities++;
                if (handHistory->at(i).action == ActionType::Raise)
                {
                    mPlayers[handHistory->at(i).playerId].mPlayerStats.postFlopStats.cBetRiver.successes++; 
                }
            }

            // If Cbet has occurred
            if (handHistory->at(i).lastStreetAggressor == handHistory->at(i).lastAggressor && numberOfBets == 1)
            {
                mPlayers[handHistory->at(i).playerId].mPlayerStats.postFlopStats.foldToCBetRiver.opportunities++;
                if (handHistory->at(i).action == ActionType::Fold)
                {
                    mPlayers[handHistory->at(i).playerId].mPlayerStats.postFlopStats.foldToCBetRiver.successes++;
                }
            }

            if (handHistory->at(i).action == ActionType::Call)
            {
                mPlayers[handHistory->at(i).playerId].mPlayerStats.postFlopStats.call.successes++;
            }
            else if (handHistory->at(i).action == ActionType::Raise)
            {
                mPlayers[handHistory->at(i).playerId].mPlayerStats.postFlopStats.betOrRaise.successes++;
                ++numberOfBets;
            }
        }
    }

    void Poker::deal()
    {
        if (!mQuiet) std::cout << "Dealing" << std::endl;
        for (int i = 0; i < mNumPlayers; ++i){
            mPlayers[i].pickUp(mDeck->draw());
            mPlayers[i].pickUp(mDeck->draw());
        }
        if (!mQuiet) printTable();
    }

    void Poker::flop()
    {
        if (!mQuiet) std::cout << "Flop" << std::endl;
        mBoard.push_back(mDeck->draw());
        mBoard.push_back(mDeck->draw());
        mBoard.push_back(mDeck->draw());
        if (!mQuiet) printTable();
    }

    void Poker::turn()
    {
        if (!mQuiet) std::cout << "Turn" << std::endl;
        mBoard.push_back(mDeck->draw());
        if (!mQuiet) printTable();
    }

    void Poker::river()
    {
        if (!mQuiet) std::cout << "River" << std::endl;
        mBoard.push_back(mDeck->draw());
        if (!mQuiet) printTable();
    }

    bool Poker::onlyOnePerson()
    {
        int count = 0;
        for (bool active : mActivePlayers)
        {
            if (active) ++count;
        }
        return count == 1;
    }

    // Doesn't deal with split pots
    int Poker::getWinner()
    {
        int winner = 0;
        long long handValue = 0;
        for (int i = 0; i < mPlayers.size(); ++i)
        {
            if (!mActivePlayers[i]) continue; //Skip the folded players

            std::vector<Card> hand = mPlayers[i].getHand();
            Hand handEval(mBoard, hand);
            if (handEval.getHandValue() > handValue)
            {
                handValue = handEval.getHandValue();
                winner = i;
            }
        }
        return winner;
    }

    double Poker::getMonteEquity(unsigned int numSims, unsigned int playerIndex)
    {
        unsigned int winCount = 0;
        int numCardsLeftToDraw = 5 - mBoard.size();
        std::vector<Card> simCards;

        for (int sims = 0; sims < numSims; ++sims)
        {
            for (int i = 0; i < numCardsLeftToDraw; ++i)
            {
                Card newCard = mDeck->draw();
                simCards.push_back(newCard);
                mBoard.push_back(newCard);
            }

            if (getWinner() == playerIndex) ++winCount;

            for (Card card : simCards)
            {
                mDeck->addBack(card);
                mBoard.pop_back();
            }

            mDeck->shuffle();
            simCards.clear();
        }

        return (double)winCount / (double)numSims;
    }

    void Poker::printTable()
    {
        std::stringstream ss;
        std::vector<Card> hand;

        ss << "Board: ";
    
        for (Card card : mBoard)
        {
            ss << card.toString() << ", ";
        }
        ss << "\n";
        ss << "Pot: " << mPot;
        ss << "\n";

        for (int i = 0; i < mPlayers.size(); ++i)
        {
            hand = mPlayers[i].getHand();
            ss << "Player " << std::to_string(i + 1) << ": (" << hand[0].toString() << ", " << hand[1].toString() << ")";

            if (mActivePlayers[i])
            {    
                Hand handEval(mBoard, hand);
                
                ss << "\tChips: " << mPlayers[i].getChips();
                ss << "\tMonte Equity: " << std::to_string(getMonteEquity(1000, i) * 100);
                ss << "\tHand value: " << std::to_string(handEval.getHandValue());
            }
            else
            {
                ss << "\tFOLDED";
            }
            ss << std::endl;
        }

        std::cout << ss.str() << std::endl;
    }
    
    void Poker::printPlayers()
    {
        std::stringstream ss;

        for (int i = 0; i < mPlayers.size(); ++i)
        {
            ss << "Player " << std::to_string(i + 1) << " chips: " << mPlayers[i].getChips();
            ss << std::endl;
        }

        std::cout << ss.str() << std::endl;
    }

    void Poker::savePlayerStats()
    {
        mPlayers[0].saveToFile("data/PlayerStats/StatsForPlayer0.txt");

        // for (Player player : mPlayers)
        // {
        //     player.saveToFile("Name.txt");
        // }
    }
}