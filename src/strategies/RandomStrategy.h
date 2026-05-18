#ifndef RANDOMSTRATEGY_H
#define RANDOMSTRATEGY_H

#include "strategies/IStrategy.h"

namespace poker
{
    class RandomStrategy : public IStrategy {
        public:
            Action decide(const GameStateView& state) override;
    };
}

#endif