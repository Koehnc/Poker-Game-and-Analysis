#ifndef MONTECARLOEVALUATOR_H
#define MONTECARLOEVALUATOR_H

#include "evaluators/IEvaluator.h"
#include "ranges/Range.h"
#include <memory>

namespace poker
{
    class MonteCarloEvaluator : public IEvaluator
    {
        public:
            // MonteCarloEvaluator() : suspectedRange(new Range("Ranges/PreflopRandomMonteEquityRange5Op.txt")) {};
            // MonteCarloEvaluator() : suspectedRange(new Range("Ranges/EqualRandomProbRange.txt")) {};
            MonteCarloEvaluator() : suspectedRange(std::make_unique<Range>("data/Ranges/Strength.txt")) {};
            double evaluate(const GameStateView& state) override;
        private:
            std::unique_ptr<Range> suspectedRange;
    };
}


#endif