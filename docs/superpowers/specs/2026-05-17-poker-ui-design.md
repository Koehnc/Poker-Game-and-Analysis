# Poker UI Design Spec
**Date:** 2026-05-17  
**Status:** Approved

## Overview

Add a Qt desktop UI that lets a human player sit at a configurable poker table and play against any combination of existing AI strategies (Random, MonteCarloV1, GAStrategy). GA genomes are trained separately via the command line and loaded into the UI as files. The game loop runs in a background QThread; the human's action is collected via a signal/slot bridge pattern.

---

## 1. GA Serialization

**New files:** `src/ga/GenomeSerializer.h`, `src/ga/GenomeSerializer.cpp`

A static utility class with two methods:

```cpp
static void save(const Genome& genome, const std::string& path);
static Genome load(const std::string& path);
```

**File format** — plain text, consistent with existing Range `.txt` files:

```
preflopAgro: 0.61
postflopAgro: 0.38
range:
0.92, 0.81, 0.74, 0.65, 0.58, 0.51, 0.44, 0.38, 0.33, 0.28, 0.24, 0.20, 0.17,
...  (13 rows of 13 comma-separated doubles)
```

**File extension:** `.genome`

**Training workflow (unchanged):** After the GA loop in `main.cpp`, call `GenomeSerializer::save(pop.best().genome, "output.genome")`. The existing per-generation range save in `Population::nextGeneration()` is kept as-is for analysis.

---

## 2. HumanStrategy

**New files:** `src/strategies/HumanStrategy.h`, `src/strategies/HumanStrategy.cpp`

Implements `IStrategy`. Bridges the synchronous game loop (game thread) with the async Qt UI (main thread).

```cpp
// In VisibleInformation.h — add after struct definitions:
Q_DECLARE_METATYPE(poker::Action)
Q_DECLARE_METATYPE(poker::GameStateView)

class HumanStrategy : public QObject, public IStrategy {
    Q_OBJECT
public:
    Action decide(const GameStateView& state) override;

public slots:
    void provideAction(Action action);   // receives action from UI thread

signals:
    void actionRequested(GameStateView state);

private:
    std::mutex mMutex;
    std::condition_variable mCv;
    Action mPendingAction;
    bool mActionReady = false;
};
```

**Thread safety:** `decide()` runs on the game thread. It emits `actionRequested`, then blocks on `mCv`. The UI's `provideAction` slot sets `mPendingAction`, sets `mActionReady = true`, and calls `mCv.notify_one()`. `decide()` unblocks and returns the action. No shared mutable state outside the mutex.

---

## 3. Qt Application Structure

**New directory:** `src/ui/`

| Class | File | Role |
|---|---|---|
| `MainWindow` | `MainWindow.h/.cpp` | Top-level `QMainWindow`. Owns a `QStackedWidget` that switches between `SetupWidget` and `GameWidget`. |
| `SetupWidget` | `SetupWidget.h/.cpp` | Full setup screen (layout B). Player slot list (add/remove, max 6). Strategy dropdown per slot: Random, MonteCarlo, GA. When GA is selected, a "Load genome…" button appears for that slot. Starting chips and big blind fields. "Start Game" button. |
| `GameWidget` | `GameWidget.h/.cpp` | Classic table layout (layout A). Opponent seats across top, community cards in center felt area, human hand + action panel at bottom. Updates via `GameController` signals. |
| `ActionPanel` | `ActionPanel.h/.cpp` | Fold / Call / Raise buttons. Raise shows a `QSpinBox` for bet amount (defaulting to pot-sized raise). Emits `actionChosen(Action)`. Disabled while it is not the human's turn. |
| `GameController` | `GameController.h/.cpp` | `QObject` that lives in a `QThread`. Owns the `Poker` instance and `HumanStrategy*`. Runs `simHand()` in a loop. Emits `stateUpdated(GameStateView)` after every action (so the table redraws) and `handComplete(int winner, int pot)` after each hand. |

**Startup sequence:**
1. `MainWindow` shows `SetupWidget`.
2. User configures table, clicks Start.
3. `MainWindow` builds the `vector<unique_ptr<IStrategy>>` from the setup choices (loading genomes via `GenomeSerializer` for GA slots).
4. Creates `GameController`, moves it to a `QThread`, wires signals/slots, starts the thread.
5. Switches `QStackedWidget` to `GameWidget`.
6. `GameController` starts looping `simHand()`.

**Signal/slot wiring (across threads — all queued connections):**
- `HumanStrategy::actionRequested` → `GameWidget::onActionRequested` (enables `ActionPanel`, shows state)
- `ActionPanel::actionChosen` → `HumanStrategy::provideAction`
- `GameController::stateUpdated` → `GameWidget::onStateUpdated`
- `GameController::handComplete` → `GameWidget::onHandComplete`

---

## 4. Game View Details

**Opponent seats (top):** Each shows strategy name, chip count, and face-down card backs. When a player folds, seat dims.

**Board (center):** Community cards drawn as colored card widgets. Shows "FLOP / TURN / RIVER" label and current pot.

**Human seat (bottom):** Shows hole cards face-up. Chip count. `ActionPanel` appears and is enabled only when it is the human's turn.

**Optional overlays (toggle buttons in toolbar):** Equity %, opponent range grid, player stats. These are out of scope for the initial build but the layout reserves space for a collapsible side panel.

---

## 5. Changes to Existing Code

| File | Change |
|---|---|
| `src/ga/Population.cpp` | No change needed — per-gen range save kept as-is. |
| `src/main.cpp` | Add `GenomeSerializer::save(pop.best().genome, "best.genome")` after the training loop. |
| `src/game/Poker.h/.cpp` | No changes. The `Poker(vector<unique_ptr<IStrategy>>, startingMoney, quiet)` constructor already exists. |
| `CMakeLists.txt` | Add `find_package(Qt6)`, enable `AUTOMOC`/`AUTOUIC`, add `src/ui/*.cpp` to sources. |

---

## 6. Build

Qt6 is required. CMakeLists.txt updated to:
```cmake
find_package(Qt6 REQUIRED COMPONENTS Widgets)
set(CMAKE_AUTOMOC ON)
target_link_libraries(poker Qt6::Widgets)
```

The existing g++ one-liner no longer works once Qt is added — CMake becomes the required build path.

---

## 7. Out of Scope (this iteration)

- In-app GA training
- Background/continuous training while playing
- Equity / range / stats overlays (architecture supports them; build deferred)
- Split pots
- Save/resume game sessions
