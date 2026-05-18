#include "game/Poker.h"
#include "core/Card.h"
#include "ranges/Range.h"
#include "evaluators/MonteCarloEvaluator.h"
#include "ga/Population.h"
#include "ga/GenomeSerializer.h"

#include <iostream>
#include <vector>

int main(int argc, char* argv[]) {

    //Control
    bool geneticAlgorithm = true;
    bool pokerGame = false;
    bool rangeCreation = false;

    // Genetic Algorithm things
    if (geneticAlgorithm)
    {
        poker::Population pop(100, 4, 10000);  // 100 individuals, 4-player tables, 10k chips
        for (int gen = 0; gen < 50; ++gen) {
            pop.evaluate(100);                 // 500 hands per individual
            pop.nextGeneration();
            std::cout << "Gen " << pop.generation() << " best fitness: " << pop.best().fitness << "\n";
        }
        poker::GenomeSerializer::save(pop.best().genome, "data/Ranges/GeneticAlgorithm/best.genome");
        std::cout << "Best genome saved to data/Ranges/GeneticAlgorithm/best.genome\n";
    }


    // Poker Sim stuff
    if (pokerGame)
    {
        int numPlayers = 4;
        std::vector<int> winCounts;
        for (int i = 0; i < numPlayers; ++i)
        {
            winCounts.push_back(0);
        }

        poker::Poker game(numPlayers, 10000, true);
        std::cout << "Starting Sim" << std::endl;
        for (int i = 0; i < 100; ++i)
        {
            game.simHand();
        }
        std::cout << std::endl;
        game.printPlayers();
        game.savePlayerStats();
    }




    // Range Creation Stuff
    if (rangeCreation)
    {
        int numTotalPlayers = 6;
        std::vector<poker::Card> hand;
        std::vector<poker::Card> board;
        poker::Range range;

        for (int r = 0; r < 13; ++r)
        {
            for (int c = 0; c < 13; ++c)
            {
                if (c > r) 
                {
                    poker::Card card1(14-r, 0);
                    poker::Card card2(14-c, 0);

                    hand.push_back(card1);
                    hand.push_back(card2);
                }
                else 
                {
                    poker::Card card1(14-c, 0);
                    poker::Card card2(14-r, 1);

                    hand.push_back(card1);
                    hand.push_back(card2);
                }

                poker::GameStateView state;
                state.board = board;
                state.playerHand = hand;
                state.numRemainingPlayers = numTotalPlayers;
                poker::MonteCarloEvaluator eval;
                double equity = eval.evaluate(state);

                std::cout << "Hand: " << hand[0].toString() << ", " << hand[1].toString() << " had equity: " << std::to_string(equity) << std::endl;

                range.updateHand(r, c, equity);
                hand.clear();
            }
        }
        range.saveToFile("HandProbability.txt");
    }

    // More Range creation things
    if (false)
    {
        // poker::Range::createPairRange("Ranges/Pairs.txt");
        // poker::Range::createSuitedRange("Ranges/Suited.txt");
        // poker::Range::createConnectedRange("Ranges/Connected.txt");
        // poker::Range::createStrengthRange("Ranges/Strength.txt");

        poker::Range range;
        // range.getHandMatrix();
    }
};

// New Strategies
    // Ranges

// Issues:
// Hands aren't equally common, the 13x13 says they are. Off suit have 6 permutations, suits have 4. 
    // Create a way to mask/combine ranges?? (Created AdjustWeights in range for it... Happy?)
    // This is relevant for how to change based off of blocked or adjusted behavior
// Check amount of players. Shouldnt? work with 2 player because blinds (Maybe it will....)
// Monte evaluator sometimes give the same result the whole time (Maybe random issue?)
// Refactor Random to be more easily controlled and monitored (Speed up too)
// Bets are allowed to be negative / having no chips doesn't eliminate you from the game
// Connected Range doesn't work with Aces... I think

// Improvements
// Strategy return a name for player file write out
// Player file write out to folder and naming convention for every player
// Read stats in for player to append too.
// Position; Does this need to be explicit?
// Update Ranges for blocking and actions
// Stats change ranges
    // RangeModifier class?
// Human controlled Player
// Refactor the winning condition in simHand
// UI? QT :)
    // Eventually visible range identification would be sick (Katherine)
// More evalautors??? Dunno

// Ideas:
// Genetic Algorithm for rule sets that define the behavior
    // Could expand the Genetic Algorithm to play certain ranges too
    // Part of the chromosome could be rule sets/arbitrary values with aggression and relative aggression
// Neural Nets could try to predict the ranges of other players and the feedback is the exact range the other person played
// Can I use convolutions on ranges?????? Prolly not... "That's a creative idea; however..."
// With the coefficient method, could I use r values (?) from some applied stats thing that would give the correlation to importance
// Plot the money of every player in a hand. Plot the range over multiple simulations