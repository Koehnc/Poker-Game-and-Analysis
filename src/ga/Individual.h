#ifndef INDIVIDUAL_H
#define INDIVIDUAL_H

#include "strategies/GAStrategy.h"

namespace poker
{
    struct Individual
    {
        Genome genome;
        double fitness = 0.0;

        //Helper Inputs
        double equity;
            /* Could Sub with effective Hand Strength */
        double handStrength;
        double percievedAggression; 
            /* Historical data of the Better. 0.0 = passive, 1.0 = aggressive */

        //Genes
        double aggression; // 0.0 = passive, 1.0 = aggressive
        double bluffFrequency;


        
    };
}

#endif