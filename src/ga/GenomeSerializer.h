#pragma once
#include "strategies/GAStrategy.h"
#include <string>

namespace poker {
    class GenomeSerializer {
    public:
        static void save(const Genome& genome, const std::string& path);
        static Genome load(const std::string& path);
    };
}
