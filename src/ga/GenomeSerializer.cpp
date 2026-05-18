#include "ga/GenomeSerializer.h"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace poker {

void GenomeSerializer::save(const Genome& genome, const std::string& path) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("GenomeSerializer: cannot open for write: " + path);
    out << std::setprecision(17);
    out << "preflopAgro: " << genome.preflopAgro << "\n";
    out << "postflopAgro: " << genome.postflopAgro << "\n";
    out << "range:\n";
    for (int i = 0; i < 13; ++i) {
        for (int j = 0; j < 13; ++j) {
            out << genome.range[i][j];
            if (j < 12) out << ", ";
        }
        out << "\n";
    }
}

Genome GenomeSerializer::load(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("GenomeSerializer: cannot open for read: " + path);
    Genome genome{};
    std::string line;

    auto parseValue = [](const std::string& l) {
        return std::stod(l.substr(l.find(':') + 1));
    };

    std::getline(in, line); genome.preflopAgro  = parseValue(line);
    std::getline(in, line); genome.postflopAgro = parseValue(line);
    std::getline(in, line); // "range:"

    for (int i = 0; i < 13; ++i) {
        std::getline(in, line);
        std::stringstream ss(line);
        std::string token;
        int j = 0;
        while (std::getline(ss, token, ',') && j < 13)
            genome.range[i][j++] = std::stod(token);
    }
    return genome;
}

} // namespace poker
