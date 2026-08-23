#include "ui/GameController.h"
#include <QThread>

namespace poker {

namespace {
    constexpr int kOpponentActionPauseMs = 900;
    constexpr int kStreetDealtPauseMs = 1300;
    constexpr int kHandCompletePauseMs = 2500;
}

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

        if (!mRunning.load()) return;   // stopping — don't pace, drain fast

        if (step.kind == StepKind::PlayerToAct && step.action.playerId != mHumanIndex) {
            QThread::msleep(kOpponentActionPauseMs);
        } else if (step.kind == StepKind::StreetDealt) {
            QThread::msleep(kStreetDealtPauseMs);
        }
    });
}

void GameController::run() {
    mRunning.store(true);
    while (mRunning.load()) {
        int winner = mPoker->simHand();

        QVector<int> chips;
        chips.reserve(mNumPlayers);
        for (int i = 0; i < mNumPlayers; ++i)
            chips.push_back(mPoker->getPlayerChips(i));

        emit handComplete(winner, chips, mPoker->getWinnerHoleCards(), mPoker->getWinningCards());
        if (mRunning.load()) {   // stopping — don't pace, drain fast
            QThread::msleep(kHandCompletePauseMs);
        }
    }
    emit gameOver();
}

void GameController::stop() {
    mRunning.store(false);
}

} // namespace poker
