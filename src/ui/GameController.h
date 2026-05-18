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
                   QObject* parent = nullptr);

public slots:
    void run();
    void stop();

signals:
    void handComplete(int winner, QVector<int> chipCounts);
    void gameOver();

private:
    std::unique_ptr<Poker> mPoker;
    int                    mNumPlayers;
    bool                   mRunning = false;
};

} // namespace poker
