
#include "ranges/RangeModifier.h"

#include <algorithm>
#include <math.h>

namespace poker
{
    RangeModifier::RangeModifier()
    {

    }

    // What Chatgpt told me (That makes sense)
    /**
     * I produced some tables for Pairs, Suited, Connecters, and Strength (Non-Monte Carlo options now)
     * Now, I should use the stats to create coefficients to effect these hands. 
     * IE: updated_range = base_range * exp(w_strength * strength_range + w_connected * connected_range + w_pairs * pairs_range...)
     */
    Range RangeModifier::updateRangeFromPlayer(Range baseRange, poker::PlayerStats playerStats)
    {
        Range range = baseRange;
        range.normalize();
        Range suitedFeatureRange("data/Ranges/Suited.txt");
        Range connectedFeatureRange("data/Ranges/Connected.txt");
        Range pairsFeatureRange("data/Ranges/Pairs.txt");
        Range strengthFeatureRange("data/Ranges/Strength.txt");

        double weights[13][13];
        double suitedFeatures[13][13];
        double connectedFeatures[13][13];
        double pairsFeatures[13][13];
        double strengthFeatures[13][13];

        for (int r = 0; r < 13; ++r)
        {
            for (int c = 0; c < 13; ++c)
            {

                weights[r][c] = range.getWeightForHand(r, c);
                suitedFeatures[r][c] = suitedFeatureRange.getWeightForHand(r, c);
                connectedFeatures[r][c] = connectedFeatureRange.getWeightForHand(r, c);
                strengthFeatures[r][c] = strengthFeatureRange.getWeightForHand(r, c);
                pairsFeatures[r][c] = pairsFeatureRange.getWeightForHand(r, c);
            }
        }
        
        double w_looseness = (playerStats.preflopStats.VPIP.pct() - 0.20) * 2.5;

        double w_strength;
        double w_pair;
        double w_suited;
        double w_connected;

        // Aggressive players play high cards more then speculative hands
        double preflopAggression = playerStats.preflopStats.raise.pct() - playerStats.preflopStats.call.pct();
        w_strength += preflopAggression * 1.5;
        w_connected -= preflopAggression * 0.5;
        w_suited    -= preflopAggression * 0.3;

        // If villian folds to threebets, he plays a lot of speculative (drawing hands)
        if (playerStats.preflopStats.foldToThreeBet.pct() < 0.4) 
        {
            w_connected += 0.5;
            w_suited    += 0.5;
        }

        for (int r = 0; r < 13; ++r)
        {
            for (int c = 0; c < 13; ++c)
            {
                // Looseness is VPIP based
                weights[r][c] = weights[r][c] * 
                    exp(w_looseness * (0.5 * suitedFeatures[r][c] + 0.5 * connectedFeatures[r][c]) +
                    w_strength * strengthFeatures[r][c] + 
                    w_pair * pairsFeatures[r][c] + 
                    w_connected * connectedFeatures[r][c] +
                    w_suited * suitedFeatures[r][c]);

            }
        }

        for (int r = 0; r < 13; ++r)
        {
            for (int c = 0; c < 13; ++c)
            {
                range.updateHand(r, c, weights[r][c]);
            }
        }

        range.normalize();
        range.saveToFile("data/Ranges/estimatedHandStrength.txt");
        return range;
        

        // Based on Actions: (Later)
        // If there was a threebet, the strength of hands increase, pairs and suited cards get bumped.
        // w_strength += threeBet * 1.2;
        // w_pair     += threeBet * 0.6;
        // w_suited   += threeBet * 0.4;

        // Never folds to cBets
        // High aggression
        // Post Flop
        // w_connected += 0.6; // chasing draws
        // w_suited    += 0.6;
        // w_pair      += 0.4; // sticky with pairs
    }

    void RangeModifier::adjustForHandProbability(Range *range)
    {

    }

    void RangeModifier::increaseChanceForSuited(Range *range, double weight)
    {

    }

    void RangeModifier::adjustForThreeBet(Range *range, double weight, double threeBetPercentage)
    {

    }

    void RangeModifier::adjustBroadway(Range *range, double weight)
    {
        for (int i = 0; i < 5; ++i)
        {
            for (int j = 0; j < 5; ++j)
            {
                range->updateHand(i, j, range->getWeightForHand(i,j) * weight);
            }
        }
    }

    void RangeModifier::adjustPairs(Range *range, double weight)
    {
        for (int i = 0; i < 13; ++i)
        {
            range->updateHand(i, i, range->getWeightForHand(i,i) * weight);
        }
    }
}