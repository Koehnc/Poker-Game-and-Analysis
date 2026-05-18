#include "ga/Population.h"
#include "game/Poker.h"
#include "strategies/GAStrategy.h"
#include "strategies/RandomStrategy.h"
#include "ranges/Range.h"

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <iostream>
#include <sstream>

namespace poker
{
    Population::Population(int size, int tableSize, int startingChips)
        : mTableSize(tableSize), mStartingChips(startingChips), mRng(std::random_device{}())
    {
        if (tableSize < 2)
            throw std::invalid_argument("tableSize must be at least 2");

        // Initialize population with random genomes via the default GAStrategy constructor
        mIndividuals.resize(size);
        for (auto& ind : mIndividuals)
        {
            GAStrategy tmp;
            ind.genome = tmp.getGenome();
        }
    }

    void Population::evaluate(int handsPerIndividual)
    {
        // Each generation simulates numIndividuals * handsPerIndividual: 100 * 500, 50,000 sims per generation
        for (auto& ind : mIndividuals)
        {
            std::vector<std::unique_ptr<IStrategy>> strategies;
            strategies.push_back(std::make_unique<GAStrategy>(ind.genome));
            for (int i = 1; i < mTableSize; ++i)
                strategies.push_back(std::make_unique<RandomStrategy>());   // Could do with different play styles

            Poker game(std::move(strategies), mStartingChips, true);
            for (int h = 0; h < handsPerIndividual; ++h)
                game.simHand();

            // Fitness = average chip gain per hand
            ind.fitness = (double)(game.getPlayerChips(0) - mStartingChips)
                          / handsPerIndividual;
        }
    }

    void Population::nextGeneration(double eliteRate, double mutationRate)
    {
        std::sort(mIndividuals.begin(), mIndividuals.end(),
            [](const Individual& a, const Individual& b) { return a.fitness > b.fitness; });

        int eliteCount = std::max(1, (int)(mIndividuals.size() * eliteRate));
        std::vector<Individual> next;
        next.reserve(mIndividuals.size());

        for (int i = 0; i < eliteCount; ++i)
            next.push_back(mIndividuals[i]);

        std::uniform_real_distribution<double> dist(0.0, 1.0);
        while ((int)next.size() < (int)mIndividuals.size())
        {
            Individual p1 = tournamentSelect();
            Individual p2 = tournamentSelect();
            auto children = GAStrategy::Crossover(p1.genome, p2.genome);
            for (auto& childGenome : children)
            {
                if ((int)next.size() >= (int)mIndividuals.size()) break;
                Individual child;
                child.genome = childGenome;
                if (dist(mRng) < mutationRate)
                    GAStrategy::Mutate(child.genome);
                next.push_back(child);
            }
        }

        mIndividuals = std::move(next);
        ++mGeneration;

        // Save the best range of the generation to a file for analysis
        poker::Range range(mIndividuals[0].genome.range);
        std::stringstream filename;
        filename << "data/Ranges/GeneticAlgorithm/" << "Gen" << std::to_string(mGeneration) << "Fitness" << std::to_string(mIndividuals[0].fitness) << ".txt";
        range.saveToFile(filename.str());
    }

    const Individual& Population::best() const
    {
        return *std::max_element(mIndividuals.begin(), mIndividuals.end(),
            [](const Individual& a, const Individual& b) { return a.fitness < b.fitness; });
    }

    Individual Population::tournamentSelect(int k) const
    {
        std::uniform_int_distribution<int> dist(0, (int)mIndividuals.size() - 1);
        Individual best = mIndividuals[dist(mRng)];
        for (int i = 1; i < k; ++i)
        {
            const Individual& candidate = mIndividuals[dist(mRng)];
            if (candidate.fitness > best.fitness)
                best = candidate;
        }
        return best;
    }
}