
#include "evaluators/MonteCarloEvaluator.h"
#include "ranges/RangeModifier.h"
#include "core/Hand.h"
#include "core/Deck.h"

namespace poker
{
    double MonteCarloEvaluator::evaluate(const GameStateView &state)
    {
        int numSims = 1000;

        RangeModifier RM;
        Range baseRange;
        // if (state.currentBetterInd != -1) baseRange = RM.updateRangeFromPlayer(*suspectedRange, state.currentBetterStats);
        // else baseRange = *suspectedRange;
        baseRange = *suspectedRange;
        Deck baseDeck;
        for (Card card : state.board)
        {
            baseDeck.remove(card);
            // baseRange.remove(card);  // The thought here is to reduce the chance of them having the card
        }
        for (Card card : state.playerHand)
        {
            baseDeck.remove(card);
            // baseRange.remove(card);  // The thought here is to reduce the chance of them having the card
        }

        std::vector<std::vector<Card>> hands;
        std::vector<Card> boardPart2;
        int boardRemaining = 5 - state.board.size();
        int winCount = 0;
        int winner = 0;
        long long handValue = 0;
        
        for (int i = 0; i < numSims; ++i)
        {
            Deck deck = baseDeck;
            deck.shuffle();

            hands.push_back(state.playerHand);
            for (int j = 0; j < state.numRemainingPlayers-1; ++j)
            {
                std::vector<Card> hand = baseRange.sampleHand();
                deck.remove(hand[0]);
                deck.remove(hand[1]);
                hands.push_back(hand);
            }

            for (int k = 0; k < boardRemaining; ++k) boardPart2.push_back(deck.draw());
            winner = 0;
            handValue = 0;

            for (int j = 0; j < hands.size(); ++j)
            {
                Hand handEval(hands[j], state.board, boardPart2);
                if (handEval.getHandValue() > handValue)
                {
                    handValue = handEval.getHandValue();
                    winner = j;
                }
            }

            if (winner == 0) ++winCount;
            hands.clear();
            boardPart2.clear();
        }

        return (double)winCount / (double)numSims;
    }
}

