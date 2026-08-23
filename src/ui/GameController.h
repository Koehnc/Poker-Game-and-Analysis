#pragma once
#include "ui/PokerQtTypes.h"
#include "game/Poker.h"
#include "ui/HumanStrategy.h"
#include <QObject>
#include <QVector>
#include <atomic>
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
    void handComplete(int winner, QVector<int> chipCounts,
                       std::vector<poker::Card> winnerHoleCards,
                       std::vector<poker::Card> winningCards);
    void gameOver();

private:
    std::unique_ptr<Poker> mPoker;
    int                    mNumPlayers;
    int                    mHumanIndex;
    std::atomic<bool>      mRunning{false};
};

} // namespace poker
