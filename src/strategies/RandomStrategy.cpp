#include "strategies/RandomStrategy.h"

#include <random>

namespace poker
{
    Action RandomStrategy::decide(const GameStateView &state)
    {
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<std::mt19937::result_type> dist3(1,3);

        Action action;
        
        switch (dist3(rng))
        {
            case 1:
                if (state.minToCall == 0)
                {
                    action.type = ActionType::Check;
                    action.amount = 0;
                }
                else
                {
                    action.type = ActionType::Fold;
                    action.amount = 0;
                }
                break;
            case 2:
                if (state.minToCall == 0)
                {
                    action.type = ActionType::Check;
                    action.amount = 0;
                }
                else if (state.minToCall <= state.playerStack)
                {
                    action.type = ActionType::Call;
                    action.amount = state.minToCall;
                }
                else
                {
                    action.type = ActionType::Call;
                    action.amount = state.playerStack;
                }
                break;
            case 3:
                if (state.minToCall * 2 + 1 <= state.playerStack)
                {
                    action.type = ActionType::Raise;
                    action.amount = state.minToCall * 2 + 1;
                }
                else
                {
                    // Doesn't really cover the case where it technically should be a call, but has the same effect
                    action.type = ActionType::Raise;
                    action.amount = state.playerStack;
                }
                break;
            default:
                return action;
        }
        return action;
    }
}