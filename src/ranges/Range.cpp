
#include "ranges/Range.h"

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <random>

namespace poker
{
    // Should delete the use of addVisit, updateHandForProbability needs to move to RangeModifier

    Range::Range()
    {
        mHands.resize(13, std::vector<double>(13, 0.0));
    }

    Range::Range(double range[13][13])
    {
        mHands.resize(13, std::vector<double>(13, 0.0));
        for (int i = 0; i < 13; ++i)
        {
            for (int j = 0; j < 13; ++j)
            {
                mHands[i][j] = range[i][j];
            }
        }
    }

    Range::Range(std::string fileName)
    {
        mHands.resize(13, std::vector<double>(13, 0.0));
        if (std::filesystem::exists(fileName))
        {
            readFromFile(fileName);
        }
    }

    void Range::updateHand(int i, int j, double weight)
    {
        mHands[i][j] = weight;
    }

    // Used so strategies can keep track of their hands played over different code executions
    void Range::addVisit(std::vector<Card> hand, std::string possibleFileName)
    {
        Card card1 = *std::max_element(hand.begin(), hand.end());
        Card card2 = *std::min_element(hand.begin(), hand.end());

        int row = 0;
        int col = 0;
        if (card1.getSuit() == card2.getSuit())
        {
            row = 14 - card1.getRank();
            col = 14 - card2.getRank();
        }
        else
        {
            row = 14 - card2.getRank();
            col = 14 - card1.getRank();
        }

        mHands[row][col] += 1.0;

        // Prolly slow, but I don't really feel like closing files at the end of runs
        if (possibleFileName != "Default")
        {
            saveToFile(possibleFileName);
        }
    }

    double Range::getWeightForHand(int i, int j)
    {
        return mHands[i][j];
    }

    double Range::adjustForHandProbability(int r, int c, double weight)
    {
        // Unsuited non-pairs have 12 permutations
        // Pairs have 6 permutations
        // Suited hands have 4 permutations

        if (r == c)
        {
            return weight * .5;
        }
        else if (r > c)
        {
            return weight * 4 / 12;
        }
        else
        {
            return weight;
        }
    }

    // This shouldn't adjust for hand Probability anymore
    std::vector<Card> Range::sampleHand()
    {
        std::vector<Card> hand;
        double sum = 0;
        for (int r = 0; r < 13; ++r)
        {
            for (int c = 0; c < 13; ++c)
            {
                sum += adjustForHandProbability(r, c, std::fmod(mHands[r][c], 1.0));
            }
        }

        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<std::mt19937::result_type> dist(0.0,sum);

        double randSelect = dist(rng);
        sum = 0;
        for (int r = 0; r < 13; ++r)
        {
            for (int c = 0; c < 13; ++c)
            {
                sum += adjustForHandProbability(r, c, std::fmod(mHands[r][c], 1.0));
                if (sum >= randSelect)
                {
                    if (r > c)  //Suited
                    {
                        hand.push_back(Card(14-r, 0));
                        hand.push_back(Card(14-c, 0));
                        return hand;
                    }
                    else
                    {
                        hand.push_back(Card(14-r, 0));
                        hand.push_back(Card(14-c, 1));
                        return hand;
                    }
                }
            }
        }
        return hand;
    }

    void Range::normalize() 
    {
        double sum = 0.0;

        // Compute total weight
        for (int i = 0; i < 13; i++) {
            for (int j = 0; j < 13; j++) {
                sum += mHands[i][j];
            }
        }

        // Avoid divide-by-zero
        if (sum <= 0.0) return;

        // Normalize
        for (int i = 0; i < 13; i++) {
            for (int j = 0; j < 13; j++) {
                mHands[i][j] /= sum;
            }
        }
    }

    double Range::rankValue(int r) 
    {
        return r / 12.0;
    }   

    double Range::pairScore(int r) 
    {
        double x = rankValue(r);
        return 0.6 + 0.4 * x;  
    }

    double Range::highCardScore(int r1, int r2) 
    {
        double high = std::max(r1, r2);
        double low  = std::min(r1, r2);

        return 0.7 * rankValue(high) + 0.3 * rankValue(low);
    }

    double Range::suitedScore(bool suited) 
    {
        return suited ? 0.08 : 0.0;
    }

    double Range::connectedScore(int r1, int r2) 
    {
        int gap = abs(r1 - r2);

        if (gap == 0) return 0.0; // pair handled separately

        return exp(-0.7 * (gap - 1));
    }

    double Range::computeStrength(int r1, int r2, bool suited) 
    {
        if (r1 == r2) {
            return pairScore(r1);
        }

        double strength =
            0.65 * highCardScore(r1, r2) +
            0.20 * connectedScore(r1, r2) +
            suitedScore(suited);

        return strength;
    }

    void Range::createPairRange(std::string filename)
    {
        Range range;
        for (int i = 0; i < 13; ++i)
        {
            range.updateHand(i, i, 1.0);
        }
        range.normalize();
        range.saveToFile(filename);
    }

    void Range::createSuitedRange(std::string filename)
    {
        Range range;
        for (int i = 0; i < 13; ++i)
        {
            for (int j = 0; j < 13; ++j)
            {
                if (j > i)
                {
                    range.updateHand(i, j, 1.0);
                }
            }
        }
        range.normalize();
        range.saveToFile(filename);
    }

    void Range::createConnectedRange(std::string filename)
    {
        Range range;
        for (int i = 0; i < 13; ++i)
        {
            for (int j = 0; j < 13; ++j)
            {
                range.updateHand(i, j, range.connectedScore(14-i, 14-j));
            }
        }
        range.normalize();
        range.saveToFile(filename);
    }

    void Range::createStrengthRange(std::string filename)
    {
        Range range;
        for (int i = 0; i < 13; ++i)
        {
            for (int j = 0; j < 13; ++j)
            {
                range.updateHand(i, j, range.computeStrength(14-i, 14-j, j > i));
            }
        }
        range.normalize();
        range.saveToFile(filename);
    }

    void Range::printRange()
    {
        for (std::vector<double> row : mHands)
        {
            for (double weight : row)
            {
                std::cout << weight << ", ";
            }
            std::cout << std::endl;
        }
    }

    void Range::saveToFile(std::string fileName)
    {
        std::ofstream outFile(fileName);
        for (std::vector<double> row : mHands)
        {
            for (double weight : row)
            {
                outFile << std::to_string(weight) << ", ";
            }
            outFile << "\n";
        }

        outFile.close();
    }

    void Range::readFromFile(std::string fileName)
    {
        std::ifstream inFile(fileName);
        std::string line;
        int row = 0;

        while (std::getline(inFile, line) && row < 13) {
            std::stringstream ss(line);
            std::string value;
            int col = 0;

            while (std::getline(ss, value, ',') && col < 13) {
                mHands[row][col] = std::stod(value);
                col++;
            }

            row++;
        }

        inFile.close();
    }
}