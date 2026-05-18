
#include "strategies/GAStrategy.h"

#include <algorithm>
#include <random>

namespace poker
{
    GAStrategy::GAStrategy() : mRng(std::random_device{}())
    {
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        for (int i = 0; i < 13; ++i)
            for (int j = 0; j < 13; ++j)
                mGenome.range[i][j] = dist(mRng);
        mGenome.preflopAgro = dist(mRng);
        mGenome.postflopAgro = dist(mRng);
    }

    GAStrategy::GAStrategy(Genome g) : mGenome(g), mRng(std::random_device{}()) {}

    Action GAStrategy::decide(const GameStateView& state)
    {
        auto& hand = state.playerHand;
        if (hand.empty()) return {ActionType::Check, 0};

        std::uniform_real_distribution<double> dist(0.0, 1.0);

        if (state.round == Round::Preflop)
        {
            // Map hand to 13x13 matrix using same convention as Player::recordHand
            // Rank 14=Ace -> index 0, Rank 2 -> index 12 (index = 14 - rank)
            Card c1 = *std::max_element(hand.begin(), hand.end());
            Card c2 = *std::min_element(hand.begin(), hand.end());
            bool suited = c1.getSuit() == c2.getSuit();
            int row, col;
            if (suited) {
                row = 14 - c1.getRank();  // higher rank -> smaller index (upper triangle: col > row)
                col = 14 - c2.getRank();
            } else {
                row = 14 - c2.getRank();  // lower rank -> larger index (lower triangle: row >= col)
                col = 14 - c1.getRank();
            }
            row = std::max(0, std::min(12, row));
            col = std::max(0, std::min(12, col));

            double playThreshold = mGenome.range[row][col];

            if (state.minToCall == 0) {
                // BB with no raise: check or raise
                if (dist(mRng) < playThreshold * mGenome.preflopAgro)
                    return {ActionType::Raise, 6};  // standard 3x open
                return {ActionType::Check, 0};
            }

            if (dist(mRng) > playThreshold)
                return {ActionType::Fold, 0};
            if (dist(mRng) < mGenome.preflopAgro)
                return {ActionType::Raise, state.minToCall * 3};
            return {ActionType::Call, state.minToCall};
        }
        else
        {
            // Postflop: pot odds vs postflopAgro threshold
            if (state.minToCall == 0) {
                if (dist(mRng) < mGenome.postflopAgro)
                    return {ActionType::Raise, std::max(state.pot / 2, 2)};
                return {ActionType::Check, 0};
            }
            double potOdds = (double)state.minToCall / (state.pot + state.minToCall);
            if (potOdds > mGenome.postflopAgro)
                return {ActionType::Fold, 0};
            return {ActionType::Call, state.minToCall};
        }
    }

    // Uniform crossover: each gene independently inherited from either parent
    std::vector<Genome> GAStrategy::Crossover(const Genome& g1, const Genome& g2)
    {
        thread_local std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<double> dist(0.0, 1.0);

        Genome c1 = g1, c2 = g2;
        for (int i = 0; i < 13; ++i)
            for (int j = 0; j < 13; ++j)
                if (dist(rng) < 0.5) std::swap(c1.range[i][j], c2.range[i][j]);

        if (dist(rng) < 0.5) std::swap(c1.preflopAgro, c2.preflopAgro);
        if (dist(rng) < 0.5) std::swap(c1.postflopAgro, c2.postflopAgro);

        return {c1, c2};
    }

    // Gaussian perturbation with 5% per-gene mutation rate
    void GAStrategy::Mutate(Genome& genome)
    {
        thread_local std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<double> uniform(0.0, 1.0);
        std::normal_distribution<double> gauss(0.0, 0.1);

        const double mutRate = 0.05;
        for (int i = 0; i < 13; ++i)
            for (int j = 0; j < 13; ++j)
                if (uniform(rng) < mutRate) {
                    genome.range[i][j] += gauss(rng);
                    genome.range[i][j] = std::max(0.0, std::min(1.0, genome.range[i][j]));
                }

        if (uniform(rng) < mutRate) {
            genome.preflopAgro += gauss(rng);
            genome.preflopAgro = std::max(0.0, std::min(1.0, genome.preflopAgro));
        }
        if (uniform(rng) < mutRate) {
            genome.postflopAgro += gauss(rng);
            genome.postflopAgro = std::max(0.0, std::min(1.0, genome.postflopAgro));
        }
    }
}