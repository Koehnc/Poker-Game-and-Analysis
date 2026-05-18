#ifndef POPULATION_H
#define POPULATION_H

#include "ga/Individual.h"
#include <vector>
#include <random>

namespace poker
{
    class Population
    {
        public:
            // tableSize: total players per evaluation game (individual + opponents)
            Population(int size, int tableSize, int startingChips);

            // Run each individual against RandomStrategy opponents and record chip delta
            void evaluate(int handsPerIndividual);

            // Select, crossover, mutate — advances one generation
            void nextGeneration(double eliteRate = 0.1, double mutationRate = 0.3);

            const Individual& best() const;
            int generation() const { return mGeneration; }
            int size() const { return (int)mIndividuals.size(); }

        private:
            Individual tournamentSelect(int k = 5) const;

            std::vector<Individual> mIndividuals;
            int mTableSize;
            int mStartingChips;
            int mGeneration = 0;
            mutable std::mt19937 mRng;
    };
}

#endif