#include "ga/GenomeSerializer.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <iostream>

int main() {
    poker::Genome g{};
    g.preflopAgro  = 0.61;
    g.postflopAgro = 0.38;
    for (int i = 0; i < 13; ++i)
        for (int j = 0; j < 13; ++j)
            g.range[i][j] = (i * 13 + j) / 169.0;

    const std::string path = "test_genome_round_trip.genome";
    poker::GenomeSerializer::save(g, path);
    poker::Genome loaded = poker::GenomeSerializer::load(path);
    std::remove(path.c_str());

    assert(std::abs(loaded.preflopAgro  - g.preflopAgro)  < 1e-9);
    assert(std::abs(loaded.postflopAgro - g.postflopAgro) < 1e-9);
    for (int i = 0; i < 13; ++i)
        for (int j = 0; j < 13; ++j)
            assert(std::abs(loaded.range[i][j] - g.range[i][j]) < 1e-9);

    std::cout << "GenomeSerializer: all assertions passed\n";
    return 0;
}
