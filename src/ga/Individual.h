#ifndef INDIVIDUAL_H
#define INDIVIDUAL_H

#include "strategies/GAStrategy.h"

namespace poker
{
    struct Individual
    {
        Genome genome;
        double fitness = 0.0;
    };
}

#endif