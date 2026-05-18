#include "strategies/MonteCarloStrategyV1.h"
#include "game/VisibleInformation.h"
#include "core/Deck.h"
#include "core/Card.h"
#include "core/Hand.h"

#include <random>
#include <iostream>

namespace poker
{
    Action MonteCarloStrategyV1::decide(const GameStateView &state)
    {
        Action action;
        double potOdds = (double)state.minToCall / (double)state.pot;
        double equity = mEvaluator->evaluate(state);

        // std::cout << "Simulated equity: " << equity << ", pot odds: " << potOdds << std::endl;

        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<std::mt19937::result_type> dist10(1,10);
        bool randomize70 = dist10(rng) <= 7;
        double loseCall = .1;   //Arbitrary value for calling more often

        if (equity >= potOdds && randomize70)
        {
            action.type = ActionType::Raise;
            action.amount = state.minToCall * 2 + 1;
            // Issue this works if there's multiple raises too.... (Not inclusize of numPlayers btw)
            if (state.board.size() == 0) personalRange->addVisit(state.playerHand, "data/Ranges/MonteCarloV1PreflopRange.txt");
        }
        else if (equity + loseCall >= potOdds)
        {
            if (state.minToCall == 0) 
            {
                action.type = ActionType::Check;
                action.amount = 0;
            }
            else
            {
                action.type = ActionType::Call;
                action.amount = state.minToCall;
                if (state.board.size() == 0) personalRange->addVisit(state.playerHand, "data/Ranges/MonteCarloV1PreflopRange.txt");
            }
        }
        else
        {
            action.type = ActionType::Fold;
            action.amount = 0;
        }

        return action;
    }
}