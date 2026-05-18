#ifndef RANGEMODIFIER_H
#define RANGEMODIFIER_H

#include "ranges/Range.h"
#include "game/VisibleInformation.h"

namespace poker
{
    class RangeModifier
    {
        public:
            RangeModifier();

            Range updateRangeFromPlayer(Range baseRange, poker::PlayerStats playerStats);

            void adjustForHandProbability(Range *range);
            void increaseChanceForSuited(Range *range, double weight);
            void adjustForThreeBet(Range *range, double weight, double threeBetPercentage);
            void adjustBroadway(Range *range, double weight);

            void adjustPairs(Range *range, double weight);

        private:
            Range mRange;
    };
}

#endif