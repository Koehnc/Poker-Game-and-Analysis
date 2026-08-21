# Live Action Feed & Position Markers Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix two UX gaps in the Qt poker UI: (1) no dealer/small-blind/big-blind markers on any seat, and (2) the table silently skips from one of your decisions straight to the next, hiding every opponent action and board card dealt in between.

**Architecture:** `Poker` (core engine, no Qt, shared with `poker-train`) gains a `dealerIndex` field on `GameStateView` and an optional `StepCallback` invoked once per hand-start, per player action, and per street deal — mirroring the pattern the original UI design spec already called for (`stateUpdated` after every action) but never implemented. `GameController` (Qt layer, runs on the background `QThread`) wires that callback to a new `stepOccurred(HandStep)` signal and inserts a short `QThread::msleep` after opponent actions and street deals — the same pacing technique already used for the 800ms pause between hands — so the human has time to watch the table before their own turn. `GameWidget` renders each step: refreshing the board/pot, updating "D"/"SB"/"BB" badges on every seat, and appending a line to a new persistent history log panel. Because each step is delivered as one discrete event (not a batched replay), a later pass can swap the instant widget updates for animations without touching the engine or threading layer again.

**Tech Stack:** C++17, Qt 6 (Widgets), existing `Poker`/`IStrategy`/`GameStateView` engine (unchanged wire format aside from the additions below).

**Spec:** No standalone spec doc — this plan is scoped directly from a systematic-debugging pass on the current uncommitted UI branch (dealer/blind data was never plumbed into `GameStateView`; `GameController` only ever emitted `handComplete` once per full hand and `HumanStrategy::actionRequested` on the human's turn, so nothing in between was ever visible) plus the user's explicit choice: live step-through with pauses now, animation-ready hooks for later, and a persistent history log.

## Global Constraints

- `src/game/*` (the core engine) must stay Qt-free — `poker-train` links it with no Qt dependency. Only `std::function`/STL may be used in `Poker.h/.cpp` and `VisibleInformation.h`.
- Every task that touches `src/game/*` must be verified by building the `poker-train` target (no Qt required, fast to check).
- Every task that touches `src/ui/*` must be verified by building the `poker-ui` target: `cd build && C:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe poker-ui -j8`.
- There is no automated GUI test harness in this repo (the only existing test is the plain-assert `test-genome` round-trip check). "Testing" for UI tasks means: it builds clean, and — for the final integration task — a manual smoke test of a live game.
- Match existing style: `mMemberName` for members, `QObject`-derived classes get `Q_OBJECT`, cross-thread signal/slot connections carry Qt value types by value (already true for `GameStateView`/`Action`).

---

## Task 1: Add `dealerIndex` and the `HandStep` event type to the core engine

**Files:**
- Modify: `src/game/VisibleInformation.h`
- Modify: `src/game/Poker.h`
- Modify: `src/game/Poker.cpp`

**Interfaces:**
- Produces: `poker::StepKind` enum (`HandStarted`, `PlayerActed`, `StreetDealt`), `poker::HandStep` struct (`kind`, `round`, `board`, `pot`, `dealerIndex`, `action`), `GameStateView::dealerIndex` (int), `Poker::StepCallback` (`std::function<void(const HandStep&)>`), `Poker::setStepCallback(StepCallback)`.
- Consumes: nothing new (builds on existing `Poker::mDealerIndex`, `mBoard`, `mPot`, `betsIn`, `simHand`).

- [ ] **Step 1: Add `StepKind`, `HandStep`, and `GameStateView::dealerIndex` to `VisibleInformation.h`**

In `src/game/VisibleInformation.h`, add the new enum and struct after the `GameStateView` struct closes (after line 88, before the final `#endif`), and add the new field to `GameStateView` itself:

```cpp
    struct GameStateView 
    {
        std::vector<Card> board;
        int pot;
        int minToCall;
        int playerPosition;
        int dealerIndex;
        int currentBetterInd;   // -1, no bet present
        PlayerStats currentBetterStats;
        int playerStack;
        std::vector<Card> playerHand;
        Round round;

        int numRemainingPlayers;    // Eventually this could be gotten rid of and the strategies can parse the actionHistory
        std::vector<ActionRecord> actionHistory;
    };

    enum class StepKind { HandStarted, PlayerActed, StreetDealt };

    struct HandStep
    {
        StepKind kind;
        Round round;
        std::vector<Card> board;
        int pot;
        int dealerIndex;
        ActionRecord action;   // only meaningful when kind == StepKind::PlayerActed
    };
}
```

(This replaces the existing `struct GameStateView { ... };` block — only the added `int dealerIndex;` line and the new `StepKind`/`HandStep` block after it are new; everything else in the struct is unchanged.)

- [ ] **Step 2: Declare the callback type and hook points in `Poker.h`**

In `src/game/Poker.h`, add `#include <functional>` to the includes, and add the public method + private members. The class becomes:

```cpp
#ifndef POKER_H
#define POKER_H

#include "game/Player.h"
#include "core/Deck.h"
#include "core/Card.h"
#include "strategies/IStrategy.h"
#include <vector>
#include <memory>
#include <random>
#include <functional>

namespace poker {
    class Poker {
        public:
            static const unsigned int BigBlind = 2;
            static const unsigned int SmallBlind = 1;

            using StepCallback = std::function<void(const HandStep&)>;

            Poker(int numPlayers, int startingMoney, bool quiet=false);
            Poker(std::vector<std::unique_ptr<IStrategy>> strategies, int startingMoney, bool quiet=false);
            void restartGame();
            int simHand(); //#ThisIsBasicallyTheRunner

            void setStepCallback(StepCallback cb);

            std::vector<ActionRecord>* betsIn(int startingPosition, poker::Round round, std::vector<ActionRecord> *streetHistory);
            void collectStats(std::vector<ActionRecord>* handHistory);
            void collectPreflopStats(std::vector<ActionRecord> *handHistory); // Could be private
            void collectFlopStats(std::vector<ActionRecord> *handHistory); // Could be private
            void collectTurnStats(std::vector<ActionRecord> *handHistory); // Could be private
            void collectRiverStats(std::vector<ActionRecord> *handHistory); // Could be private

            void deal();
            void flop();
            void turn();
            void river();

            bool onlyOnePerson();
            int getWinner();
            double getMonteEquity(unsigned int numSims, unsigned int playerIndex);
            int getPlayerChips(int playerIndex) const;

            void printTable();
            void printPlayers();
            void savePlayerStats();

        private:
            void emitStep(StepKind kind, Round round, const ActionRecord* action = nullptr);

            int mNumPlayers;
            std::vector<Player> mPlayers;
            std::vector<bool> mActivePlayers;
            std::vector<int> mBetsFromPlayers;
            std::unique_ptr<Deck> mDeck;
            bool mQuiet;

            int mPot;
            int mMinCall;
            int mDealerIndex;
            int mCurrentBetter;
            int mPreviousStreetAggressor;
            std::vector<Card> mBoard;
            StepCallback mStepCallback;
    };
}

#endif
```

- [ ] **Step 3: Implement `setStepCallback`/`emitStep` and call them from `Poker.cpp`**

In `src/game/Poker.cpp`, add the two new methods anywhere in the `poker` namespace block (e.g. right after `getPlayerChips`, around line 75):

```cpp
    void Poker::setStepCallback(StepCallback cb)
    {
        mStepCallback = std::move(cb);
    }

    void Poker::emitStep(StepKind kind, Round round, const ActionRecord* action)
    {
        if (!mStepCallback) return;
        HandStep step;
        step.kind = kind;
        step.round = round;
        step.board = mBoard;
        step.pot = mPot;
        step.dealerIndex = mDealerIndex;
        if (action) step.action = *action;
        mStepCallback(step);
    }
```

Then wire the four call sites. In `simHand()` (around line 99), add an `emitStep` call right after `deal()`, and after each of `flop()`, `turn()`, `river()`:

```cpp
        deal();
        emitStep(StepKind::HandStarted, poker::Round::Preflop);

        if (!mQuiet) std::cout << "First round bets" << std::endl;
        betsIn(mDealerIndex + 3, poker::Round::Preflop, &streetHistory);
        if (onlyOnePerson())
        {
            winner = getWinner();
            collectStats(&streetHistory);
            mPlayers[winner].payout(mPot);
            return winner;
        }

        flop();
        emitStep(StepKind::StreetDealt, poker::Round::Flop);

        if (!mQuiet) std::cout << "Second round bets" << std::endl;
        betsIn(mDealerIndex + 1, poker::Round::Flop, &streetHistory);
        if (onlyOnePerson())
        {
            winner = getWinner();
            collectStats(&streetHistory);
            mPlayers[winner].payout(mPot);
            return winner;
        }

        turn();
        emitStep(StepKind::StreetDealt, poker::Round::Turn);

        if (!mQuiet) std::cout << "Third round bets" << std::endl;
        betsIn(mDealerIndex + 1, poker::Round::Turn, &streetHistory);
        if (onlyOnePerson())
        {
            winner = getWinner();
            collectStats(&streetHistory);
            mPlayers[winner].payout(mPot);
            return winner;
        }

        river();
        emitStep(StepKind::StreetDealt, poker::Round::River);

        if (!mQuiet) std::cout << "Final round bets" << std::endl;
        betsIn(mDealerIndex + 1, poker::Round::River, &streetHistory);
        winner = getWinner();
        collectStats(&streetHistory);
        if (!mQuiet) printTable();

        mPlayers[winner].payout(mPot);
        return winner;
```

In `betsIn()` (around line 157), set `state.dealerIndex` alongside the other `state.*` assignments, and emit a `PlayerActed` step right after the action is recorded into `streetHistory`:

```cpp
            GameStateView state;
            state.board = mBoard;
            state.minToCall = mMinCall - mBetsFromPlayers[j];
            state.playerPosition = j;
            state.dealerIndex = mDealerIndex;
            state.currentBetterInd = mCurrentBetter;
            if (mCurrentBetter != -1) state.currentBetterStats = mPlayers[mCurrentBetter].mPlayerStats;
            state.playerHand = mPlayers[j].getHand();
            state.pot = mPot;
            state.numRemainingPlayers = numActivePlayers;
            state.round = round;

            Action action = mPlayers[j].getAction(state);
            int betSize = action.amount;

            streetHistory->emplace_back(ActionRecord{ j, round, action.type, action.amount, mPot, mPreviousStreetAggressor, mCurrentBetter } );
            emitStep(StepKind::PlayerActed, round, &streetHistory->back());
```

- [ ] **Step 4: Build `poker-train` to confirm the core engine still compiles Qt-free**

```bash
cd build && C:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe poker-train -j8
```

Expected: builds with no errors (this target has no Qt dependency, so any accidental Qt include would fail here first).

- [ ] **Step 5: Commit**

```bash
git add src/game/VisibleInformation.h src/game/Poker.h src/game/Poker.cpp
git commit -m "feat: add dealer position and per-step callback to core engine"
```

---

## Task 2: Wire `GameController` to emit `stepOccurred` with pacing

**Files:**
- Modify: `src/ui/PokerQtTypes.h`
- Modify: `src/ui/GameController.h`
- Modify: `src/ui/GameController.cpp`

**Interfaces:**
- Consumes: `poker::HandStep`, `poker::StepKind` from Task 1.
- Produces: `GameController::stepOccurred(poker::HandStep)` signal; `GameController` constructor now takes `int humanIndex` as its third parameter (before `parent`).

- [ ] **Step 1: Register `HandStep` as a Qt metatype**

In `src/ui/PokerQtTypes.h`, add the new declaration:

```cpp
#pragma once
#include "game/VisibleInformation.h"
#include <QMetaType>

Q_DECLARE_METATYPE(poker::Action)
Q_DECLARE_METATYPE(poker::GameStateView)
Q_DECLARE_METATYPE(poker::HandStep)
```

- [ ] **Step 2: Add `humanIndex` and the `stepOccurred` signal to `GameController.h`**

Replace the class body in `src/ui/GameController.h`:

```cpp
#pragma once
#include "ui/PokerQtTypes.h"
#include "game/Poker.h"
#include "ui/HumanStrategy.h"
#include <QObject>
#include <QVector>
#include <memory>
#include <vector>

namespace poker {

class GameController : public QObject {
    Q_OBJECT
public:
    GameController(std::vector<std::unique_ptr<IStrategy>> strategies,
                   int startingChips,
                   int humanIndex,
                   QObject* parent = nullptr);

public slots:
    void run();
    void stop();

signals:
    void stepOccurred(poker::HandStep step);
    void handComplete(int winner, QVector<int> chipCounts);
    void gameOver();

private:
    std::unique_ptr<Poker> mPoker;
    int                    mNumPlayers;
    int                    mHumanIndex;
    bool                   mRunning = false;
};

} // namespace poker
```

- [ ] **Step 3: Set the step callback and add pacing in `GameController.cpp`**

Replace `src/ui/GameController.cpp`:

```cpp
#include "ui/GameController.h"
#include <QThread>

namespace poker {

GameController::GameController(std::vector<std::unique_ptr<IStrategy>> strategies,
                               int startingChips,
                               int humanIndex,
                               QObject* parent)
    : QObject(parent),
      mNumPlayers(static_cast<int>(strategies.size())),
      mHumanIndex(humanIndex)
{
    mPoker = std::make_unique<Poker>(std::move(strategies), startingChips, /*quiet=*/true);

    mPoker->setStepCallback([this](const HandStep& step) {
        emit stepOccurred(step);

        if (step.kind == StepKind::PlayerActed && step.action.playerId != mHumanIndex) {
            QThread::msleep(600);
        } else if (step.kind == StepKind::StreetDealt) {
            QThread::msleep(800);
        }
    });
}

void GameController::run() {
    mRunning = true;
    while (mRunning) {
        int winner = mPoker->simHand();

        QVector<int> chips;
        chips.reserve(mNumPlayers);
        for (int i = 0; i < mNumPlayers; ++i)
            chips.push_back(mPoker->getPlayerChips(i));

        emit handComplete(winner, chips);
        QThread::msleep(800);
    }
    emit gameOver();
}

void GameController::stop() {
    mRunning = false;
}

} // namespace poker
```

The lambda runs synchronously inside `Poker::betsIn`/`simHand`, which execute on the background `QThread` that `GameController` is moved to — so `QThread::msleep` here pauses only the game loop, never the UI thread, exactly like the existing 800ms pause between hands.

- [ ] **Step 4: Build `poker-ui`**

This will fail to link until Task 4 updates the `GameController(...)` call site in `MainWindow.cpp` — that's expected. For now, confirm the file compiles in isolation:

```bash
cd build && C:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe poker-ui -j8 2>&1 | grep -i "GameController\|MainWindow"
```

Expected: the only errors mention `MainWindow.cpp` calling `GameController(...)` with the wrong number of arguments — `GameController.cpp`/`.h` themselves show no errors. This gets fixed in Task 4.

- [ ] **Step 5: Commit**

```bash
git add src/ui/PokerQtTypes.h src/ui/GameController.h src/ui/GameController.cpp
git commit -m "feat: emit stepOccurred signal with pacing from GameController"
```

---

## Task 3: Render position badges and a history log in `GameWidget`

**Files:**
- Modify: `src/ui/GameWidget.h`
- Modify: `src/ui/GameWidget.cpp`

**Interfaces:**
- Consumes: `poker::HandStep`, `poker::StepKind`, `GameStateView::dealerIndex` from Tasks 1–2.
- Produces: `GameWidget::onStepOccurred(poker::HandStep)` slot (new); `GameWidget::onActionRequested` now also updates badges via the shared `updateBadges`/`refreshBoard` helpers.

- [ ] **Step 1: Replace `src/ui/GameWidget.h`**

```cpp
#pragma once
#include "ui/PokerQtTypes.h"
#include "ui/ActionPanel.h"
#include <QWidget>
#include <QVector>

class QLabel;
class QFrame;
class QListWidget;
class QVBoxLayout;
class QHBoxLayout;

namespace poker {

class GameWidget : public QWidget {
    Q_OBJECT
public:
    explicit GameWidget(QStringList playerNames, int humanIndex, QWidget* parent = nullptr);

public slots:
    void onActionRequested(poker::GameStateView state);
    void onStepOccurred(poker::HandStep step);
    void onHandComplete(int winner, QVector<int> chipCounts);

signals:
    void actionChosen(poker::Action action);

private:
    static QFrame* makeCardWidget(QWidget* parent);
    void setCardFace(QFrame* card, const Card& c);
    void setCardBack(QFrame* card);
    void clearCard(QFrame* card);
    void refreshBoard(int pot, const std::vector<Card>& board);
    void updateBadges(int dealerIndex);
    void logAction(const ActionRecord& record);
    void logStreet(Round round, const std::vector<Card>& board);
    static QString cardShortText(const Card& c);

    int          mHumanIndex;
    QStringList  mPlayerNames;
    QVector<int> mOpponentIndices;   // actual player index for each entry in mChipLabels/mSeatBadges

    QVector<QLabel*>  mChipLabels;       // one per opponent, in order
    QVector<QLabel*>  mSeatBadges;       // one per opponent, parallel to mChipLabels
    QLabel*           mHumanBadge   = nullptr;
    QVector<QFrame*>  mBoardCards;       // 5 community card slots
    QVector<QFrame*>  mHumanCards;       // 2 hole card slots
    QLabel*           mPotLabel      = nullptr;
    QLabel*           mHumanChipsLabel = nullptr;
    QLabel*           mStatusLabel   = nullptr;
    ActionPanel*      mActionPanel   = nullptr;
    QListWidget*      mHistoryLog    = nullptr;
};

} // namespace poker
```

- [ ] **Step 2: Replace `src/ui/GameWidget.cpp`**

```cpp
#include "ui/GameWidget.h"
#include "core/Card.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QListWidget>

namespace poker {

// ─── Card widget helpers ───────────────────────────────────────────────────

QFrame* GameWidget::makeCardWidget(QWidget* parent) {
    auto* frame = new QFrame(parent);
    frame->setFixedSize(52, 74);
    auto* label = new QLabel("", frame);
    label->setObjectName("cardLabel");
    label->setGeometry(0, 0, 52, 74);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("background: transparent;");
    return frame;
}

QString GameWidget::cardShortText(const Card& c) {
    QString rank;
    switch (c.getRank()) {
        case 14: rank = "A"; break;
        case 13: rank = "K"; break;
        case 12: rank = "Q"; break;
        case 11: rank = "J"; break;
        default: rank = QString::number(c.getRank());
    }
    QString suit;
    switch (c.getSuit()) {
        case 0: suit = "♣"; break;
        case 1: suit = "♦"; break;
        case 2: suit = "♥"; break;
        case 3: suit = "♠"; break;
        default: suit = "?";
    }
    return rank + suit;
}

void GameWidget::setCardFace(QFrame* card, const Card& c) {
    QString rank;
    switch (c.getRank()) {
        case 14: rank = "A"; break;
        case 13: rank = "K"; break;
        case 12: rank = "Q"; break;
        case 11: rank = "J"; break;
        default: rank = QString::number(c.getRank());
    }

    QString suit, color;
    switch (c.getSuit()) {
        case 0: suit = "♣"; color = "#1a1a1a"; break;  // clubs
        case 1: suit = "♦"; color = "#c0202a"; break;  // diamonds
        case 2: suit = "♥"; color = "#c0202a"; break;  // hearts
        case 3: suit = "♠"; color = "#1a1a1a"; break;  // spades
        default: suit = "?";     color = "#555";    break;
    }

    card->setStyleSheet(
        "QFrame { background-color: #f5f0e8; border: 1px solid #c8c0b0; border-radius: 8px; }"
    );

    auto* label = card->findChild<QLabel*>("cardLabel");
    if (!label) return;
    label->setStyleSheet(QString(
        "background: transparent;"
        "color: %1;"
        "font-family: Georgia;"
        "font-weight: bold;"
    ).arg(color));
    label->setText(QString(
        "<div style='font-size:15px; line-height:1;'>%1</div>"
        "<div style='font-size:21px; line-height:1;'>%2</div>"
    ).arg(rank, suit));
    label->setTextFormat(Qt::RichText);
}

void GameWidget::setCardBack(QFrame* card) {
    card->setStyleSheet(
        "QFrame { background-color: #1a2a5a; border: 1px solid #2a3a80; border-radius: 8px; }"
    );
    auto* label = card->findChild<QLabel*>("cardLabel");
    if (!label) return;
    label->setStyleSheet("background: transparent; color: #2a3a70; font-size: 26px;");
    label->setText("★");
    label->setTextFormat(Qt::PlainText);
}

void GameWidget::clearCard(QFrame* card) {
    card->setStyleSheet(
        "QFrame { background-color: transparent; border: 1px dashed #1a2a1a; border-radius: 8px; }"
    );
    auto* label = card->findChild<QLabel*>("cardLabel");
    if (label) { label->setText(""); }
}

// ─── Constructor ───────────────────────────────────────────────────────────

GameWidget::GameWidget(QStringList playerNames, int humanIndex, QWidget* parent)
    : QWidget(parent), mHumanIndex(humanIndex), mPlayerNames(playerNames)
{
    auto* outer = new QHBoxLayout(this);
    outer->setSpacing(14);
    outer->setContentsMargins(20, 16, 20, 14);

    auto* tableCol = new QWidget(this);
    auto* root = new QVBoxLayout(tableCol);
    root->setSpacing(10);
    root->setContentsMargins(0, 0, 0, 0);

    // ── Opponent seats (top row) ───────────────────────────────────────────
    auto* opponentRow = new QHBoxLayout;
    opponentRow->setSpacing(12);

    for (int i = 0; i < playerNames.size(); ++i) {
        if (i == humanIndex) continue;

        auto* seat = new QFrame(tableCol);
        seat->setObjectName("opponentSeat");

        auto* sl = new QVBoxLayout(seat);
        sl->setSpacing(5);
        sl->setContentsMargins(14, 12, 14, 12);

        auto* badge = new QLabel("", seat);
        badge->setObjectName("seatBadge");
        badge->setAlignment(Qt::AlignCenter);
        badge->setVisible(false);

        auto* nameL = new QLabel(playerNames[i], seat);
        nameL->setObjectName("nameLabel");
        nameL->setAlignment(Qt::AlignCenter);

        auto* chipL = new QLabel("$10000", seat);
        chipL->setObjectName("chipLabel");
        chipL->setAlignment(Qt::AlignCenter);

        // Two face-down cards
        auto* cardRow = new QHBoxLayout;
        cardRow->setSpacing(5);
        cardRow->setAlignment(Qt::AlignCenter);
        auto* c1 = makeCardWidget(seat);
        auto* c2 = makeCardWidget(seat);
        setCardBack(c1);
        setCardBack(c2);
        cardRow->addWidget(c1);
        cardRow->addWidget(c2);

        sl->addWidget(badge);
        sl->addWidget(nameL);
        sl->addWidget(chipL);
        sl->addLayout(cardRow);

        opponentRow->addWidget(seat);
        mChipLabels.push_back(chipL);
        mSeatBadges.push_back(badge);
        mOpponentIndices.push_back(i);
    }
    root->addLayout(opponentRow);

    // ── Board (center oval) ────────────────────────────────────────────────
    auto* boardFrame = new QFrame(tableCol);
    boardFrame->setObjectName("boardFrame");

    auto* boardLayout = new QVBoxLayout(boardFrame);
    boardLayout->setSpacing(10);
    boardLayout->setContentsMargins(28, 18, 28, 18);

    mPotLabel = new QLabel("POT  —  $0", boardFrame);
    mPotLabel->setObjectName("potLabel");
    mPotLabel->setAlignment(Qt::AlignCenter);

    auto* cardRowLayout = new QHBoxLayout;
    cardRowLayout->setSpacing(10);
    cardRowLayout->setAlignment(Qt::AlignCenter);
    for (int i = 0; i < 5; ++i) {
        auto* card = makeCardWidget(boardFrame);
        clearCard(card);
        mBoardCards.push_back(card);
        cardRowLayout->addWidget(card);
    }

    boardLayout->addWidget(mPotLabel);
    boardLayout->addLayout(cardRowLayout);
    root->addWidget(boardFrame, 1);

    // ── Status bar ─────────────────────────────────────────────────────────
    auto* statusBar = new QFrame(tableCol);
    statusBar->setObjectName("statusBar");
    auto* statusLayout = new QHBoxLayout(statusBar);
    statusLayout->setContentsMargins(0, 0, 0, 0);
    mStatusLabel = new QLabel("Waiting for hand to begin...", statusBar);
    mStatusLabel->setObjectName("statusLabel");
    mStatusLabel->setAlignment(Qt::AlignCenter);
    statusLayout->addWidget(mStatusLabel);
    root->addWidget(statusBar);

    // ── Human seat (bottom) ────────────────────────────────────────────────
    auto* humanFrame = new QFrame(tableCol);
    humanFrame->setObjectName("humanSeat");

    auto* humanLayout = new QHBoxLayout(humanFrame);
    humanLayout->setSpacing(18);
    humanLayout->setContentsMargins(20, 14, 20, 14);

    mHumanBadge = new QLabel("", humanFrame);
    mHumanBadge->setObjectName("seatBadge");
    mHumanBadge->setAlignment(Qt::AlignCenter);
    mHumanBadge->setVisible(false);

    auto* handRow = new QHBoxLayout;
    handRow->setSpacing(8);
    handRow->setAlignment(Qt::AlignCenter);
    for (int i = 0; i < 2; ++i) {
        auto* card = makeCardWidget(humanFrame);
        setCardBack(card);
        mHumanCards.push_back(card);
        handRow->addWidget(card);
    }

    mHumanChipsLabel = new QLabel(
        QString("You (%1)   $10000").arg(playerNames[humanIndex]),
        humanFrame);
    mHumanChipsLabel->setObjectName("chipLabel");
    mHumanChipsLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    humanLayout->addWidget(mHumanBadge);
    humanLayout->addLayout(handRow);
    humanLayout->addWidget(mHumanChipsLabel, 1);
    root->addWidget(humanFrame);

    // ── Action panel ───────────────────────────────────────────────────────
    mActionPanel = new ActionPanel(tableCol);
    root->addWidget(mActionPanel);

    connect(mActionPanel, &ActionPanel::actionChosen, this, &GameWidget::actionChosen);

    outer->addWidget(tableCol, 1);

    // ── History log (right panel) ──────────────────────────────────────────
    mHistoryLog = new QListWidget(this);
    mHistoryLog->setObjectName("historyLog");
    mHistoryLog->setFixedWidth(220);
    outer->addWidget(mHistoryLog);
}

// ─── Shared update helpers ──────────────────────────────────────────────────

void GameWidget::refreshBoard(int pot, const std::vector<Card>& board) {
    mPotLabel->setText(QString("POT  —  $%1").arg(pot));
    for (int i = 0; i < 5; ++i) {
        if (i < static_cast<int>(board.size()))
            setCardFace(mBoardCards[i], board[i]);
        else
            clearCard(mBoardCards[i]);
    }
}

void GameWidget::updateBadges(int dealerIndex) {
    int n = mPlayerNames.size();
    auto badgeFor = [dealerIndex, n](int playerIndex) -> QString {
        int offset = ((playerIndex - dealerIndex) % n + n) % n;
        if (offset == 0) return "D";
        if (offset == 1) return "SB";
        if (offset == 2) return "BB";
        return "";
    };

    for (int k = 0; k < mOpponentIndices.size(); ++k) {
        QString text = badgeFor(mOpponentIndices[k]);
        mSeatBadges[k]->setText(text);
        mSeatBadges[k]->setVisible(!text.isEmpty());
    }

    QString humanText = badgeFor(mHumanIndex);
    mHumanBadge->setText(humanText);
    mHumanBadge->setVisible(!humanText.isEmpty());
}

void GameWidget::logAction(const ActionRecord& record) {
    QString name = (record.playerId < mPlayerNames.size()) ? mPlayerNames[record.playerId] : "Player";
    QString line;
    switch (record.action) {
        case ActionType::Fold:  line = QString("%1 folds").arg(name); break;
        case ActionType::Check: line = QString("%1 checks").arg(name); break;
        case ActionType::Call:  line = QString("%1 calls $%2").arg(name).arg(record.amount); break;
        case ActionType::Raise: line = QString("%1 raises to $%2").arg(name).arg(record.amount); break;
    }
    mHistoryLog->addItem(line);
    mHistoryLog->scrollToBottom();
}

void GameWidget::logStreet(Round round, const std::vector<Card>& board) {
    QString label;
    int newCardCount = 0;
    switch (round) {
        case Round::Flop:  label = "Flop";  newCardCount = 3; break;
        case Round::Turn:  label = "Turn";  newCardCount = 1; break;
        case Round::River: label = "River"; newCardCount = 1; break;
        default: return;
    }
    QStringList cardStrs;
    int start = static_cast<int>(board.size()) - newCardCount;
    for (int i = start; i < static_cast<int>(board.size()); ++i)
        cardStrs << cardShortText(board[i]);
    mHistoryLog->addItem(QString("%1: %2").arg(label, cardStrs.join(' ')));
    mHistoryLog->scrollToBottom();
}

// ─── Slots ─────────────────────────────────────────────────────────────────

void GameWidget::onActionRequested(poker::GameStateView state) {
    refreshBoard(state.pot, state.board);
    updateBadges(state.dealerIndex);

    // Hole cards
    for (int i = 0; i < 2; ++i) {
        if (i < static_cast<int>(state.playerHand.size()))
            setCardFace(mHumanCards[i], state.playerHand[i]);
        else
            setCardBack(mHumanCards[i]);
    }

    mStatusLabel->setText("Your turn");
    mActionPanel->activate(state.minToCall, state.pot, state.playerStack);
}

void GameWidget::onStepOccurred(poker::HandStep step) {
    refreshBoard(step.pot, step.board);
    updateBadges(step.dealerIndex);

    switch (step.kind) {
        case StepKind::HandStarted:
            mHistoryLog->clear();
            mHistoryLog->addItem("— New hand —");
            break;
        case StepKind::StreetDealt:
            logStreet(step.round, step.board);
            break;
        case StepKind::PlayerActed:
            logAction(step.action);
            break;
    }
}

void GameWidget::onHandComplete(int winner, QVector<int> chipCounts) {
    mActionPanel->deactivate();

    QString name = (winner < mPlayerNames.size()) ? mPlayerNames[winner] : "Unknown";
    mStatusLabel->setText(QString("%1 wins the hand!").arg(name));

    // Reset board to empty
    for (auto* card : mBoardCards) clearCard(card);
    for (auto* card : mHumanCards) setCardBack(card);

    // Update chip labels
    int opponentIdx = 0;
    for (int i = 0; i < mPlayerNames.size(); ++i) {
        if (i == mHumanIndex) {
            mHumanChipsLabel->setText(
                QString("You (%1)   $%2").arg(mPlayerNames[i]).arg(chipCounts[i]));
        } else if (opponentIdx < mChipLabels.size()) {
            mChipLabels[opponentIdx++]->setText(QString("$%1").arg(chipCounts[i]));
        }
    }
}

} // namespace poker
```

- [ ] **Step 3: Build `poker-ui`**

Same caveat as Task 2 Step 4 — `MainWindow.cpp` isn't updated yet, so expect only errors about the `GameController(...)` call site and the missing `stepOccurred` connection, nothing from `GameWidget.cpp` itself:

```bash
cd build && C:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe poker-ui -j8 2>&1 | grep -i "GameWidget"
```

Expected: no output (no errors reference `GameWidget.cpp`/`.h`).

- [ ] **Step 4: Commit**

```bash
git add src/ui/GameWidget.h src/ui/GameWidget.cpp
git commit -m "feat: render dealer/blind badges and a history log in GameWidget"
```

---

## Task 4: Wire `MainWindow` and register the new metatype

**Files:**
- Modify: `src/ui/MainWindow.cpp`
- Modify: `src/ui/main_ui.cpp`

**Interfaces:**
- Consumes: `GameController(strategies, startingChips, humanIndex, parent)` from Task 2, `GameWidget::onStepOccurred` from Task 3.

- [ ] **Step 1: Pass `humanIndex` to `GameController` and connect `stepOccurred`**

In `src/ui/MainWindow.cpp`, change line 82 from:

```cpp
    mController = new GameController(std::move(strategies), startingChips);
```

to:

```cpp
    mController = new GameController(std::move(strategies), startingChips, humanIndex);
```

And add a new connection alongside the existing ones (after line 88, `connect(mController, &GameController::gameOver, ...)`):

```cpp
    connect(mGameThread,  &QThread::started,              mController,  &GameController::run);
    connect(mController,  &GameController::stepOccurred,  mGameWidget,  &GameWidget::onStepOccurred);
    connect(mController,  &GameController::handComplete,  mGameWidget,  &GameWidget::onHandComplete);
    connect(mController,  &GameController::gameOver,      mGameThread,  &QThread::quit);
```

- [ ] **Step 2: Register `HandStep` as a metatype in `main_ui.cpp`**

In `src/ui/main_ui.cpp`, add the registration line after the existing `qRegisterMetaType<poker::GameStateView>(...)` call:

```cpp
    qRegisterMetaType<poker::Action>("poker::Action");
    qRegisterMetaType<poker::GameStateView>("poker::GameStateView");
    qRegisterMetaType<poker::HandStep>("poker::HandStep");
    qRegisterMetaType<poker::PlayerConfig>("poker::PlayerConfig");
    qRegisterMetaType<QVector<poker::PlayerConfig>>("QVector<poker::PlayerConfig>");
```

- [ ] **Step 3: Build `poker-ui` clean**

```bash
cd build && C:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe poker-ui -j8
```

Expected: `[100%] Built target poker-ui` with no errors.

- [ ] **Step 4: Manual smoke test**

```bash
./build/poker-ui.exe
```

Set up a table with yourself and at least 2 AI opponents, click Start Game, and verify:
- A "D" badge appears on exactly one seat, "SB" on the next, "BB" on the next (dealer rotates seat-to-seat each hand).
- After you act, the history log panel on the right starts filling in with opponent actions ("Bob calls $20", "Alice raises to $60", etc.) and street reveals ("Flop: A♠ K♦ 7♣") one at a time, with a brief pause between each, before the action panel re-enables for your next turn.
- The board/pot in the center update in lockstep with each log line rather than jumping straight to the final pre-your-turn state.
- Chip counts and the "wins the hand" message still work as before at hand end.

- [ ] **Step 5: Commit**

```bash
git add src/ui/MainWindow.cpp src/ui/main_ui.cpp
git commit -m "feat: wire stepOccurred through MainWindow for live action feed"
```

---

## Task 5: Style the badges and history log to match the dark-luxury theme

**Files:**
- Modify: `src/ui/Theme.h`

**Interfaces:**
- Consumes: `QLabel#seatBadge` and `QListWidget#historyLog` object names from Task 3.

- [ ] **Step 1: Append the new QSS rules**

In `src/ui/Theme.h`, insert the following block into the stylesheet string, right before the closing `)");` (currently line 280) — e.g. right after the existing `/* ──────────────────────────── Scrollbar ──────────────────────────────── */` block:

```css
/* ────────────────────────────── Badges ─────────────────────────────────── */

QLabel#seatBadge {
    font-size: 10px;
    font-weight: bold;
    color: #0c0c11;
    background-color: #c4991f;
    border-radius: 8px;
    padding: 2px 7px;
    min-width: 16px;
    max-width: 26px;
}

/* ─────────────────────────── History log ───────────────────────────────── */

QListWidget#historyLog {
    background-color: #111118;
    border: 1px solid #1e2030;
    border-radius: 10px;
    padding: 8px;
}
QListWidget#historyLog::item {
    color: #a8a294;
    font-size: 12px;
    padding: 3px 2px;
    border: none;
}
```

- [ ] **Step 2: Build and visually confirm**

```bash
cd build && C:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe poker-ui -j8 && ./poker-ui.exe
```

Expected: the "D"/"SB"/"BB" badges render as small gold pills matching the existing gold accent color used elsewhere (raise button, title), and the history log panel has a dark background consistent with the rest of the table rather than Qt's default white list styling.

- [ ] **Step 3: Commit**

```bash
git add src/ui/Theme.h
git commit -m "style: theme dealer/blind badges and history log panel"
```
