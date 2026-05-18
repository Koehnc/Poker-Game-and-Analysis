#ifndef GASTRATEGY_H
#define GASTRATEGY_H

#include "strategies/IStrategy.h"
#include <random>
#include <vector>

namespace poker
{
    struct Genome
    {
        double range[13][13];   // could be 3d with position as a dimension
        double preflopAgro; // raise / call
        double postflopAgro;
    };

    class GAStrategy : public IStrategy
    {
        public:
            static std::vector<Genome> Crossover(const Genome& g1, const Genome& g2);
            static void Mutate(Genome& genome);

            GAStrategy();
            GAStrategy(Genome g);
            Action decide(const GameStateView& state) override;

            const Genome& getGenome() const { return mGenome; }

        private:
            Genome mGenome;
            std::mt19937 mRng;
    };
}

/**
 * ChatGPT:
 * If you want something that works without exploding complexity:

    Preflop:
    Fixed table, evolved
    Postflop NN:
    Inputs:
    equity vs range
    pot size
    stack size
    position
    board texture
    Outputs:
    fold / call / bet size
    Evaluation:
    1,000+ hands per individual
    rotating opponents
 */

 /**
  * Use Relative Stats like stack to pot ratio
  * Could evolve some threshold values instead of using some as inputs...
  */

#endif