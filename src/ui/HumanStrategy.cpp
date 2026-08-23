#include "ui/HumanStrategy.h"

namespace poker {

HumanStrategy::HumanStrategy(QObject* parent) : QObject(parent) {}

Action HumanStrategy::decide(const GameStateView& state) {
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mActionReady = false;
    }
    emit actionRequested(state);

    std::unique_lock<std::mutex> lock(mMutex);
    mCv.wait(lock, [this] { return mActionReady; });
    return mPendingAction;
}

void HumanStrategy::provideAction(poker::Action action) {
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mPendingAction  = action;
        mActionReady    = true;
    }
    mCv.notify_one();
}

void HumanStrategy::abort() {
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mAborted = true;
        mActionReady = true;
        mPendingAction = Action{ActionType::Fold, 0};
    }
    mCv.notify_one();
}

} // namespace poker
