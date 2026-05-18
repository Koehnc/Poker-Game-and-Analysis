#ifndef RANGE_H
#define RANGE_H

#include "core/Card.h"

#include <vector>
#include <string>

namespace poker
{
    class Range
    {   
        public:
            Range();
            Range(double range[13][13]);
            Range(std::string fileName);
            void updateHand(int i, int j, double weight);
            void addVisit(std::vector<Card> hand, std::string possibleFileName = "Default");
            double getWeightForHand(int i, int j);
            void normalize();

            std::vector<Card> sampleHand();


            static void createPairRange(std::string filename);
            static void createSuitedRange(std::string filename);
            static void createConnectedRange(std::string filename);
            static void createStrengthRange(std::string filename);

            void printRange();
            void saveToFile(std::string fileName);
            void readFromFile(std::string fileName);

        private:
            std::vector<std::vector<double>> mHands;
            double adjustForHandProbability(int r, int c, double weight);

            // Used in static Range creations
            double rankValue(int r);
            double pairScore(int r);
            double highCardScore(int r1, int r2);
            double suitedScore(bool suited);
            double connectedScore(int r1, int r2);
            double computeStrength(int r1, int r2, bool suited);
    };
}

#endif