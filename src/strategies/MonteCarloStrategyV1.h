#ifndef MONTECARLOSTRATEGYV1_H
#define MONTECARLOSTRATEGYV1_H

#include "strategies/IStrategy.h"
#include "evaluators/IEvaluator.h"
#include "ranges/Range.h"

#include <memory>

namespace poker
{
    class MonteCarloStrategyV1 : public IStrategy
    {
        public:
            MonteCarloStrategyV1(std::unique_ptr<IEvaluator> eval) :
                mEvaluator(std::move(eval)),
                personalRange(std::make_unique<Range>("data/Ranges/MonteCarloV1PreflopRange.txt")) {}
            Action decide(const GameStateView& state) override;
        private:
            std::unique_ptr<IEvaluator> mEvaluator;
            std::unique_ptr<Range> personalRange;
    };
}

#endif