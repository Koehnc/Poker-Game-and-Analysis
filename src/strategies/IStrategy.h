#ifndef ISTRATEGY_H
#define ISTRATEGY_H

#include "game/VisibleInformation.h"

namespace poker
{
    class IStrategy {
        public:
            virtual Action decide(const GameStateView& state) = 0;
            virtual ~IStrategy() = default;
    };
}

#endif