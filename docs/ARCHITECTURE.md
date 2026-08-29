# Codebase Reference

A file-by-file guide to the poker bot project, current as of the `live-action-feed` merge. This is meant to be read alongside the source, not instead of it — it explains *why* things are structured the way they are and flags anything that looks fragile or unfinished, honestly rather than diplomatically.

## The big picture

Two independent binaries share almost all their source:

- **`poker-train`** — a console app (no Qt) that runs GA training, quiet batch poker sims, or preflop-range generation, gated by hand-edited `bool` flags at the top of `main.cpp` (no CLI args).
- **`poker-ui`** — the Qt6 desktop app. Only built `if(Qt6_FOUND)`.
- **`test-genome`** — a hand-rolled smoke test (see [Testing](#testing) below).

Both real binaries link the exact same `CORE_SOURCES` glob (`src/core`, `src/game`, `src/strategies`, `src/evaluators`, `src/ranges`, `src/ga`) — the UI is a strict superset dependency-wise, adding only `src/ui/*.cpp` and Qt Widgets. Nothing in `CORE_SOURCES` knows Qt exists.

**Build mechanics worth knowing:** `include_directories(src)` is global, so every `#include "core/Card.h"` is relative to `src/`. Sources are found via `file(GLOB_RECURSE ...)` — adding a new `.cpp` file won't be picked up until you re-run `cmake`, since there's no automatic reconfigure trigger.

## Layered dependency shape

Strictly bottom-up, no circular or upward dependencies:

```
core/        Card, Deck, Hand — pure value types + hand evaluation. Depends on nothing.
  ↓
game/        VisibleInformation (data types), Player, Poker — the engine.
             Depends on core/ and strategies/IStrategy.h only (for the type it runs).
  ↓
strategies/  evaluators/  ranges/     — siblings, depend on core/ + game/VisibleInformation.h
             evaluators/ additionally depends on ranges/ (owns a Range)
  ↓
ga/          Individual, Population, GenomeSerializer — depends on BOTH strategies/ and
             game/Poker.h directly (Population::evaluate spins up whole Poker games)
  ↓
ui/          depends on everything below it. Nothing outside ui/ depends on ui/.
             The only layer with a Qt dependency.
```

The one clean interface seam in the whole project is **`IStrategy`** (`decide(const GameStateView&) -> Action`). Every bot and the human player implement exactly this — `HumanStrategy` in `src/ui/` is a `QObject` *and* an `IStrategy`, which is how a human plugs into the same engine that otherwise only knows about bots.

---

## `src/core/` — fundamental representation

### Card.h / Card.cpp
`Card(int rank, int suit)` — rank is 2–14 (14 = Ace), suit is 0=Club, 1=Diamond, 2=Heart, 3=Spade.

**Watch out:** `operator<`/`operator>`/`operator==` compare **rank only**, not suit. Intentional for sorting during hand evaluation, but it means two cards of the same rank compare equal regardless of suit. `GameWidget::sameCard()` in the UI has to work around this explicitly with its own rank+suit comparison — there's a comment there calling it out.

### Deck.h / Deck.cpp
Wraps `std::vector<Card>` of all 52. `shuffle()` reseeds `std::mt19937` from `std::random_device` on every call (wasteful, not wrong). `draw()` pops from the back with **no empty-check** — will misbehave on an empty deck. `addBack()` restores a card (used by Monte Carlo simulation to undo speculative deals).

**Fixed (was a real bug):** `remove(Card card)` used to do `mCards.erase(mCards.begin(), cardInd)` — erasing *everything from the front up to* the matched card instead of just the one card, and since `Card::operator==` only compares rank, the lookup itself could match the wrong suit too. `remove()` now uses `std::find_if` with an explicit rank+suit comparison and erases just that single element. This is called from `MonteCarloEvaluator::evaluate()` to strip known cards out of the simulation deck before sampling opponents — the old behavior likely skewed Monte Carlo equity estimates. Still not covered by any test.

### Hand.h / Hand.cpp
Constructors take 1–3 card vectors (2 hole + up to 3 board pieces), concatenated and sorted ascending by rank.

`calcHandValue()` returns one `long long` encoding category + tiebreakers: checked in descending strength order (straight flush → quads → full house → flush → straight → trips → two pair → pair → high card), category gets a big fixed offset (e.g. straight flush = `+80,000,000,000`) so any category beats any weaker category regardless of kickers, with kickers packed into the low digits via `getHighCardValue()`.

Known rough edges, both self-documented in code comments:
- **`getTwoPair`**: comment reads *"Fucks up when there's 3 pairs (Takes the lowest two)"* — doesn't correctly pick the top two pairs when three pairs are present.
- **`getThreePair`**: actually detects three-of-a-kind (trips), despite the name — used internally by `getFullHouse` and trip-scoring. Misleading name, not a functional bug.
- Straight detection handles the wheel (A-2-3-4-5) by re-inserting the Ace at the front and treating it as rank 1 via `rank % 14 + 1`.

`getBestFive()` (added this session) brute-forces all C(7,5)=21 five-card subsets via `std::prev_permutation` over a boolean selector, re-scoring each with a fresh `Hand`. Used only to find the winning 5 cards for the UI's showdown reveal — not used anywhere in the hot evaluation path.

---

## `src/game/` — the engine

### VisibleInformation.h
Pure data/enums, no logic, included almost everywhere:

- `ActionType` (Fold/Call/Raise/Check), `Round` (Preflop/Flop/Turn/River)
- `Stat{opportunities, successes}` — atomic unit of the stats system, with a `pct()` helper
- `PreflopStats` / `PostFlopStats` — per-street stat buckets. `PostFlopStats::checkRaise` is declared but explicitly commented "trappy action, not implemented"
- `PlayerStats{playerId, preflopStats, postFlopStats}` — carried per-`Player`, exposed to opponents via `GameStateView.currentBetterStats` so a strategy could react to a specific opponent's tendencies (the intended consumer, `RangeModifier`, is currently dead code — see below)
- `Action{type, amount}` — what `IStrategy::decide()` returns
- `ActionRecord{playerId, round, action, amount, pot, lastAggressor, lastStreetAggressor}` — one row of hand history
- `GameStateView` — the full read-only snapshot every strategy programs against: board, pot, minToCall, position, dealer, current-better + their stats, own stack/hand, round, remaining players, accumulated action history
- `StepKind{HandStarted, PlayerToAct, PlayerActed, StreetDealt}` + `HandStep{kind, round, board, pot, dealerIndex, action}` — the UI-facing live event type (distinct from `ActionRecord`, which is historical)

### Player.h / Player.cpp
Owns chips + an `IStrategy`. `getAction()` fills `state.playerStack` before delegating to the strategy, then deducts `action.amount` unconditionally — **no clamp**, it trusts the strategy to size legally. `bet()` is the real chip-committing primitive (clamps to available chips for all-ins).

`recordHand()` buckets hole cards into a 13×13 preflop grid using the convention `row/col = 14 - rank`, suited hands in the upper triangle, offsuit in the lower. **This exact convention is independently reimplemented three times** (here, in `Range::addVisit`, and in `GAStrategy::decide`) without being factored into a shared helper — a de facto protocol nobody wrote down.

**Latent bug:** `saveToFile()` prints the strategy type via `typeid(mStrategy).name()` — since `mStrategy` is `unique_ptr<IStrategy>`, this always prints the smart-pointer's type, never the concrete strategy class. Needs `typeid(*mStrategy)` for the polymorphic type.

### Poker.h / Poker.cpp
The engine core — `simHand()` is literally commented `#ThisIsBasicallyTheRunner`.

- Two constructors: one hardcodes every seat to `MonteCarloStrategyV1` (used by nothing in the UI path), one takes a `vector<unique_ptr<IStrategy>>` directly (what the UI and GA training actually use — lets you mix strategies including `HumanStrategy` per seat).
- `restartGame()` — clears board/pot, rotates dealer, resets bets, posts blinds.
- `simHand()` — deals, emits `HandStarted`, runs `betsIn()` per street, checking `onlyOnePerson()` after *every* street (early-return with payout if only one player remains). At a genuine showdown (river reached with >1 active player) it captures `mWinnerHoleCards`/`mWinningCards` via `Hand::getBestFive` for the UI — deliberately **not** populated on an uncontested fold-win.
- **`betsIn()`** — the per-street betting loop. For each active player: builds a `GameStateView`, emits `PlayerToAct` *before* asking for a decision (so the UI can highlight/pause on the current actor before they move), calls `getAction()`, records the `ActionRecord`, updates pot/bets, and on a Raise resets the iteration cursor so betting continues around the table from the raiser (`startingPosition = j; i = j;`) — a manual re-entrant loop rather than a queue. Then emits `PlayerActed`.
- **The step-callback system** — the single most important architectural seam. `setStepCallback(StepCallback cb)` stores a `std::function<void(const HandStep&)>`; `emitStep()` invokes it *synchronously, inline*, from whatever thread called `simHand()`. The engine has zero knowledge of Qt, threading, or pacing — it just fires a synchronous callback per micro-event. `GameController` (UI layer) is entirely responsible for turning that into paced, cross-thread signal emission (see below). Clean decoupling point.
- `collectStats()` and friends — post-hand pass incrementing `Stat` counters. The flop/turn/river variants are near-identical copy-pasted functions rather than one parameterized by `Round`.
- `getWinner()` — **comment: "Doesn't deal with split pots"** — ties broken arbitrarily by iteration order.
- `getMonteEquity()` — a separate, simpler built-in equity estimator used only by debug `printTable()` output, not by any strategy (don't confuse with `evaluators/MonteCarloEvaluator`).
- `savePlayerStats()` — hardcoded to only save player 0's file; the loop over all players is commented out.

---

## `src/strategies/` — decision-making

### IStrategy.h
`virtual Action decide(const GameStateView&) = 0`. The one interface everything hangs off.

### RandomStrategy.h/.cpp
Stateless. Rolls 1–3: fold-or-check, call-or-check(-or-all-in), or raise to `minToCall*2+1`. The baseline opponent — every non-individual seat in GA training is one of these, and it's the default second seat in `SetupWidget`.

### MonteCarloStrategyV1.h/.cpp
Owns an injected `IEvaluator` (normally `MonteCarloEvaluator`) and a `Range` loaded from `data/Ranges/MonteCarloV1PreflopRange.txt`. Computes pot odds vs. equity, raises 70% of the time when ahead, calls with a `+0.1` equity fudge factor ("arbitrary value for calling more often", per its own comment), else folds. Every preflop call/raise re-saves its range to disk via `Range::addVisit` — flagged by `Range` itself as slow. A comment admits unfinished multi-raise handling.

### GAStrategy.h/.cpp
The evolved strategy. `Genome{double range[13][13], preflopAgro, postflopAgro}`.

- **Preflop**: looks up `playThreshold = range[row][col]` (same 14-minus-rank convention as `Player::recordHand`, independently reimplemented). Unopposed → raise to a hardcoded `6` (not scaled to blind size) with probability `playThreshold * preflopAgro`. Facing a bet → fold if `random > playThreshold`, else raise 3x with probability `preflopAgro`, else call.
- **Postflop**: unopposed → raise `max(pot/2, 2)` with probability `postflopAgro`. Facing a bet → fold or call based on pot odds vs. `postflopAgro` — **there is no postflop-raise gene at all**, so a GA-trained bot never raises when facing a postflop bet. A real behavioral gap, not a bug.
- `Crossover()` — uniform crossover, each of the 169 range cells + 2 scalars independently 50/50 per child.
- `Mutate()` — Gaussian perturbation (σ=0.1) at 5% per-gene rate, clamped to [0,1].
- Two large trailing comments (attributed "ChatGPT:") sketch an unimplemented future neural-net postflop design — aspirational notes left in place, not code.

---

## `src/evaluators/` — equity estimation

### IEvaluator.h
`virtual double evaluate(const GameStateView&) = 0`. Consumed only by `MonteCarloStrategyV1`.

### MonteCarloEvaluator.h/.cpp
Loads a fixed opponent-sampling distribution from `data/Ranges/Strength.txt` (commented-out alternate constructors show this used to be swappable). `evaluate()` runs 1000 sims: strips known cards from a fresh deck via `Deck::remove`, samples opponent hands from the range, deals remaining board cards, counts hero wins.

**Dead code:** the per-opponent range adjustment via `RangeModifier` is commented out (`// if (state.currentBetterInd != -1) baseRange = RM.updateRangeFromPlayer(...)`) — despite `RangeModifier` being fully implemented, the evaluator always uses the static range regardless of who's actually betting. This is the main compute cost in the engine: O(1000 sims × N hand evaluations) per decision, per player, per street.

---

## `src/ga/` — genetic algorithm training

### Individual.h
Trivial: `{Genome genome; double fitness = 0.0;}`. `Genome` itself lives in `strategies/GAStrategy.h`, not here.

### Population.h/.cpp
`Population(size, tableSize, startingChips)`. `evaluate(handsPerIndividual)`: each individual plays a table of itself + `RandomStrategy` opponents for N hands (always seat 0); `fitness = (finalChips - startingChips) / handsPerIndividual`. `nextGeneration()`: keeps the top `eliteRate` unchanged, fills the rest via tournament selection (k=5) + `GAStrategy::Crossover` + probabilistic `Mutate` per child. **Writes the current best genome's range grid to a timestamped file** (`data/Ranges/GeneticAlgorithm/Gen{N}Fitness{fitness}.txt`) after every generation — this is why that directory has dozens of snapshot files from a prior real training run.

### GenomeSerializer.h/.cpp
The one thing under direct test. Human-readable text format: `preflopAgro:`, `postflopAgro:`, then 13 lines of 13 comma-separated doubles (`setprecision(17)` for round-trip fidelity). `load()` has no error recovery — a malformed file throws uncaught from `std::stod`, which `MainWindow::onGameStartRequested` does catch and show as a `QMessageBox`.

**Note:** the per-generation snapshot files from `Population::nextGeneration` are range-only, in a *different* format — **not directly loadable as genomes**. Only the final `GenomeSerializer::save(pop.best().genome, "...best.genome")` call at the end of a training run in `main.cpp` produces a file `SetupWidget`'s "Load genome..." picker can actually use.

---

## `src/ranges/` — preflop ranges

### Range.h/.cpp
`mHands` is a 13×13 weight grid, same convention as everywhere else. `addVisit()` increments a cell and immediately re-saves the whole grid to disk if given a filename — flagged in its own top-of-file comment as needing cleanup ("Should delete the use of addVisit..."). `sampleHand()` draws via cumulative-sum roulette over `fmod(weight, 1.0)` — the `fmod` strips any integer part, which only makes sense because `addVisit` accumulates whole-number visit counts that need discarding; a fragile implicit contract between the two methods. Static factories (`createPairRange`, `createSuitedRange`, etc.) generate the canned range files under `data/Ranges/` — their call sites in `main.cpp` are commented out, so they're one-off generator scripts left in place rather than part of the regular run path.

### RangeModifier.h/.cpp
Meant to reshape a base range from an opponent's observed stats. Loads four range files from disk **on every call** (not cached), computes weight coefficients from VPIP/aggression stats, reweights every cell, normalizes, saves to `data/Ranges/estimatedHandStrength.txt` (overwritten every call, not tagged per-player).

**Real bug, currently dormant:** `w_strength`, `w_pair`, `w_suited`, `w_connected` are declared with **no initializer**, and `w_strength += ...` reads it before any assignment — undefined behavior. `w_pair` is never assigned at all before being used in `exp(...)`. This is currently harmless only because the one call site (`MonteCarloEvaluator::evaluate()`) is commented out — it'll bite immediately if that line is ever re-enabled. Three other methods (`adjustForHandProbability`, `increaseChanceForSuited`, `adjustForThreeBet`) are declared with **empty bodies** — stubs, never implemented.

---

## `src/main.cpp` — console entry point

Three mutually-exclusive `bool` flags at the top gate three modes (hand-edited, no CLI parsing):

1. **GA training** (currently the active branch) — 100 individuals, 4-player tables, 10000 starting chips, 50 generations × 100 hands = 500,000 simulated hands per run, saving `data/Ranges/GeneticAlgorithm/best.genome` at the end.
2. **Poker sim** — 100 quiet hands, prints chip totals, calls `savePlayerStats()`.
3. **Range creation** — brute-force equity computation across a full 13×13 hand grid to regenerate `Strength.txt`-style tables.

A trailing plain-text comment block functions as an informal project journal — genuinely worth reading: notes on hand-count-weighting inaccuracy, unverified heads-up blind handling, *"Monte evaluator sometimes give the same result the whole time (Maybe random issue?)"*, negative bets not prevented, split-pot handling reiterated as missing, and forward-looking ideas (neural nets predicting opponent ranges, visible range identification).

---

## `src/ui/` — the Qt6 desktop app

### PokerQtTypes.h
`Q_DECLARE_METATYPE` registrations for `Action`, `GameStateView`, `HandStep`, `std::vector<Card>` — required for these types to cross Qt's queued cross-thread signal/slot boundary. (`PlayerConfig`/`QVector<PlayerConfig>` are registered separately, colocated with their definition in `SetupWidget.h` instead of here — a minor inconsistency.)

### Theme.h
`applyTheme(QApplication&)` — one big QSS stylesheet (dark theme, gold accents, green felt gradient). Purely presentational.

### main_ui.cpp
Entry point: creates `QApplication`, applies the Fusion style + theme, calls `qRegisterMetaType` for every cross-thread type (needed at *runtime* in addition to the compile-time `Q_DECLARE_METATYPE`), constructs and shows `MainWindow`, runs `app.exec()`.

### HumanStrategy.h/.cpp
Implements `IStrategy` *and* is a `QObject` — the human-input bridge, and the key cross-thread synchronization pattern in the UI. `decide()` runs on the **worker thread** (called from inside `Poker::betsIn`): it `emit actionRequested(state)` (auto-queued to the GUI thread), then **blocks** on a `std::condition_variable` until `provideAction(Action)` unblocks it. `provideAction()` is a slot connected to `GameWidget::actionChosen` — when you click Fold/Call/Raise, that runs synchronously on the GUI thread and notifies the CV, unblocking the worker. `abort()` force-unblocks a stuck `decide()` with a synthetic Fold, used on window teardown so a game mid-human-turn doesn't hang the shutdown.

### GameController.h/.cpp
**The thread-boundary owner.** Meant to be `moveToThread()`'d onto a dedicated `QThread` (done in `MainWindow`). Owns the actual `Poker` instance. Its step-callback lambda does two things: `emit stepOccurred(step)` (auto-queued cross-thread), then — only if still running — sleeps the **calling thread** (the worker thread, since the callback runs synchronously inside `Poker::betsIn`) for a pacing delay:

- `kOpponentActionPauseMs = 900` after a non-human `PlayerToAct` (pause *before* the bot's move resolves)
- `kStreetDealtPauseMs = 1300` after a `StreetDealt`
- `kHandCompletePauseMs = 2500` between hands, skipped if stopping (so shutdown drains fast)

This is the entire mechanism behind "watch the bots think and act at human speed" — the engine emits instantly, and this layer injects real wall-clock delays into the engine's own thread between events, with the engine itself never knowing pacing exists.

`stop()` just flips an atomic flag — a **rough edge**: there's no early-exit hook inside `simHand`/`betsIn`, so stopping mid-hand skips the *between-event* pacing delays but still runs the current hand to completion before the loop notices `mRunning` is false. Clicking "leave game" mid-hand isn't instant.

### MainWindow.h/.cpp
Top-level window, a `QStackedWidget` alternating `SetupWidget` ↔ `GameWidget`. `onGameStartRequested()` builds the strategy vector from `PlayerConfig`s (Human → `new HumanStrategy()`, tracked separately as `mHumanStrat`; Random/MonteCarlo → direct construction; GA → `GenomeSerializer::load()` wrapped in try/catch with a `QMessageBox` on failure), constructs `GameController` + `QThread`, calls `moveToThread()` — the explicit thread-boundary-establishing call — and wires the full signal graph (`QThread::started → run`, `stepOccurred → onStepOccurred`, `handComplete → onHandComplete`, `gameOver → QThread::quit`, `actionRequested → onActionRequested`, `actionChosen → provideAction`), then `start()`s the thread.

`stopGame()` is the teardown: aborts `mHumanStrat` first (unblocks any waiting `decide()`), stops the controller, `quit()` + `wait(3000)` the thread with a `terminate()` fallback if it doesn't exit in time — a real last-resort safety valve, inherently unsafe but reasonable given the CV-wait pattern it's guarding against.

### GameWidget.h/.cpp
The largest UI file — the actual table view. Builds a `QGridLayout` "ring" (2×3 grid, 5 named slots: top-left/top-center/top-right/left/right) for opponent seats, symmetric for 1–5 opponents via a lookup table keyed by opponent count. A central `boardFrame` (radial-gradient felt oval) holds the pot label + 5 community cards. Human seat + `ActionPanel` sit below. A sidebar holds a scrolling action-history log and a persistent outcome-bubble log of past hand results.

Per-seat state lives in **parallel `QVector`s indexed by opponent slot** (`mOpponentIndices[k]` maps back to the real player index; `mChipLabels`, `mSeatBadges`, `mOpponentCard1`, `mOpponentCard2` are all parallel to it). `opponentCardWidget(playerId, cardIndex)` does the lookup.

Three slots, all driven by queued cross-thread signals from `GameController`:
- **`onActionRequested`** (human's turn only) — reveals hole cards, activates `ActionPanel`, glows the human's cards
- **`onStepOccurred`** — the general live-feed switch: `HandStarted` resets the whole table; `StreetDealt` logs the new cards; `PlayerToAct` sets status text + glows the actor; `PlayerActed` logs the action and, on a Fold, immediately clears that player's cards
- **`onHandComplete`** — sets the win message, updates chips, and — only on a genuine showdown (`winningCards` non-empty) — reveals the winner's hole cards if they were hidden, and glows the specific cards (board and/or hole) that made the winning hand, matched by exact rank+suit (working around `Card::operator==`)

Deliberately, the board/cards are **not** cleared in `onHandComplete` — that happens on the *next* hand's `HandStarted`, so the result stays visible through the whole pause instead of vanishing the instant the hand ends.

### ActionPanel.h/.cpp
Fold/Call-or-Check/Raise controls. `activate()` computes a default raise size, relabels Call↔Check based on `minToCall`, and disables Fold specifically when `minToCall == 0` (folding when you can check free isn't a real choice).

### SetupWidget.h/.cpp
Pre-game config screen. `PlayerConfig{StrategyType, genomePath, isHuman}`. Defaults to 2 seats (human + one Random AI). Dynamically builds a row per seat; GA-strategy rows show a genome file picker only when GA is selected. Caps at 6 seats, floors at 2, refuses to remove the human's seat. `onStartClicked()` validates every GA seat has a genome path before emitting `gameStartRequested`.

---

## Testing

There is exactly **one test in the entire repository**: `tests/test_genome_serializer.cpp`, a hand-rolled `main()` (no framework, not wired into `ctest`/`enable_testing()` — build it and run it manually) that round-trips a `Genome` through `GenomeSerializer::save`/`load` and asserts every field survives within `1e-9` via plain `assert()`. Since nothing disables `NDEBUG` explicitly, these assertions would silently no-op in a release build.

Nothing exercises `Card`, `Deck`, `Hand`, `Poker`, any strategy, `Range`, `RangeModifier`, or `Population` — which is exactly where the real bugs documented below (`RangeModifier`'s uninitialized doubles, `Hand::getTwoPair`'s three-pair handling) actually live, and where `Deck::remove`'s now-fixed bug went unnoticed for however long.

---

## Known issues (as of this doc)

**Confirmed bugs:**
- `RangeModifier::updateRangeFromPlayer()` reads uninitialized `double`s — currently dormant since its only call site is commented out.
- `Hand::getTwoPair()` picks the wrong pairs when three pairs are present.
- `Player::saveToFile()` prints the wrong type via `typeid` on a `unique_ptr`.
- Hand doesn't end when action folds around to the last player (tracked separately, not yet root-caused).

**Fixed:**
- `Deck::remove()` used to erase the wrong range and could match the wrong suit (rank-only comparison) — now removes exactly the matched card via an explicit rank+suit lookup.

**Known gaps / unfinished:**
- No split-pot handling anywhere (`getWinner()` says so explicitly).
- GA-trained bots never raise postflop when facing a bet — no gene for it.
- `RangeModifier`'s per-opponent range adjustment is fully implemented but never actually used (dead code, its call site is commented out).
- Duplicate strategy names aren't disambiguated in the UI (two "Random" opponents both show up as "Random").
- No way to leave a running game and get back to the setup screen — the cleanup path exists (`MainWindow::stopGame()`) but nothing in the UI calls it.
- Stopping a game mid-hand isn't instant — the current hand finishes before the loop notices.
- Test coverage is a single serialization round-trip; nothing else in the engine is tested.
