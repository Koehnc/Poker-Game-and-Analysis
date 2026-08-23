#pragma once
#include "ui/PokerQtTypes.h"
#include "strategies/IStrategy.h"
#include <QObject>
#include <mutex>
#include <condition_variable>

namespace poker {

class HumanStrategy : public QObject, public IStrategy {
    Q_OBJECT
public:
    explicit HumanStrategy(QObject* parent = nullptr);
    Action decide(const GameStateView& state) override;
    void abort();

public slots:
    void provideAction(poker::Action action);

signals:
    void actionRequested(poker::GameStateView state);

private:
    std::mutex              mMutex;
    std::condition_variable mCv;
    Action                  mPendingAction{};
    bool                    mActionReady = false;
    bool                    mAborted = false;
};

} // namespace poker
