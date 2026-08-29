# Poker Game and Analysis

A C++ passion project exploring different approaches to poker-playing bots — a hand-crafted heuristic strategy, a Monte Carlo equity estimator, and a genetic algorithm that evolves its own preflop/postflop tendencies — plus a Qt6 desktop UI to play against them directly.

See [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) for a full file-by-file breakdown of how everything fits together, including known bugs and unfinished pieces.

## What's here

- A from-scratch poker engine (deck, hand evaluation, betting rounds, pot management) with no external game-logic dependencies.
- Three bot strategies behind a shared `IStrategy` interface: `RandomStrategy`, `MonteCarloStrategyV1` (equity-based via simulation), and `GAStrategy` (genome-driven, trainable).
- A genetic algorithm training pipeline (`src/ga/`) that evolves `GAStrategy` genomes against a population of opponents over many generations.
- A Qt6 desktop UI (`poker-ui`) to configure a table (mixing human and bot seats) and play live, with a paced action feed, position badges, and a showdown reveal of the winning hand.

## Requirements

- **Qt 6.8.3** (the `mingw_64` kit) — provides both the Widgets library and the bundled MinGW-w64 toolchain the project is built with.
- **CMake 3.16+** (tested with 4.3.2).

Full toolchain details, exact paths, and why the *specific* bundled compiler matters (not just "any MinGW") are in [`docs/REQUIREMENTS.md`](docs/REQUIREMENTS.md) — worth reading before your first build, since picking up the wrong compiler builds fine but produces a binary that silently fails to launch.

## Building

There are three targets:

- **`poker-train`** — console app, no Qt needed. Runs GA training, batch simulations, or range generation depending on which mode is enabled in `src/main.cpp` (hand-edited `bool` flags, no CLI args yet).
- **`poker-ui`** — the Qt6 desktop app. Only built if Qt6 is found by CMake.
- **`test-genome`** — a small smoke test for genome serialization round-tripping.

```powershell
cd build
& "C:\Qt\Tools\mingw1310_64\bin\cmake.exe" .. -G "MinGW Makefiles" `
    -DCMAKE_CXX_COMPILER="C:/Qt/Tools/mingw1310_64/bin/g++.exe" `
    -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/mingw_64"
& "C:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe" poker-train poker-ui test-genome -j8
```

(If `build/` is already configured — check `build/CMakeCache.txt` — you can skip straight to the `mingw32-make` step.)

## Running

```powershell
cd build
.\poker-ui.exe        # desktop app — configure a table and play
.\poker-train.exe     # console — whichever mode is active in src/main.cpp
.\test-genome.exe     # genome serialization smoke test
```

## Project layout

```
src/core/         Card, Deck, Hand — fundamental representation and hand evaluation
src/game/         Poker, Player, VisibleInformation — the engine
src/strategies/   IStrategy + Random / MonteCarlo / GA bot implementations
src/evaluators/   Monte Carlo equity estimation
src/ranges/       Preflop range representation
src/ga/           Genetic algorithm training (Population, GenomeSerializer)
src/ui/           Qt6 desktop app
data/             Trained ranges, GA generation snapshots, player stats
docs/             Architecture reference, requirements, and design plans
```

For what each file actually does, its key classes, and how it connects to the rest — including an honest list of known bugs and unfinished pieces — see [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).
