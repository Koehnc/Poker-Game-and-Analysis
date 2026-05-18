#include "ui/GameController.h"
#include <QThread>

namespace poker {

GameController::GameController(std::vector<std::unique_ptr<IStrategy>> strategies,
                               int startingChips,
                               QObject* parent)
    : QObject(parent),
      mNumPlayers(static_cast<int>(strategies.size()))
{
    mPoker = std::make_unique<Poker>(std::move(strategies), startingChips, /*quiet=*/true);
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
