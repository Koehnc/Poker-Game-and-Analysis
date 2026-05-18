#ifndef IEVALUATOR_H
#define IEVALUATOR_H

#include "game/VisibleInformation.h"

namespace poker
{
    class IEvaluator 
    {
        public:
            virtual double evaluate(const GameStateView& state) = 0;
            virtual ~IEvaluator() = default;
    };
}

#endif