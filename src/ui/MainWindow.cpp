#include "ui/MainWindow.h"
#include "ui/GameWidget.h"
#include "ui/GameController.h"
#include "ui/HumanStrategy.h"
#include "strategies/RandomStrategy.h"
#include "strategies/MonteCarloStrategyV1.h"
#include "strategies/GAStrategy.h"
#include "evaluators/MonteCarloEvaluator.h"
#include "ga/GenomeSerializer.h"
#include <QStackedWidget>
#include <QThread>
#include <QFileInfo>
#include <QMessageBox>
#include <memory>

namespace poker {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Poker");
    resize(800, 600);

    mStack       = new QStackedWidget(this);
    mSetupWidget = new SetupWidget(this);
    mStack->addWidget(mSetupWidget);
    setCentralWidget(mStack);

    connect(mSetupWidget, &SetupWidget::gameStartRequested,
            this,         &MainWindow::onGameStartRequested);
}

MainWindow::~MainWindow() {
    stopGame();
}

void MainWindow::onGameStartRequested(QVector<poker::PlayerConfig> players, int startingChips) {
    stopGame();

    std::vector<std::unique_ptr<IStrategy>> strategies;
    QStringList names;
    int humanIndex = -1;

    for (int i = 0; i < players.size(); ++i) {
        const auto& cfg = players[i];
        if (cfg.isHuman) {
            humanIndex  = i;
            auto* hs    = new HumanStrategy();  // owned by unique_ptr below
            mHumanStrat = hs;
            strategies.push_back(std::unique_ptr<IStrategy>(hs));
            names.push_back("You");
        } else {
            switch (cfg.strategy) {
                case PlayerConfig::StrategyType::Random:
                    strategies.push_back(std::make_unique<RandomStrategy>());
                    names.push_back("Random");
                    break;
                case PlayerConfig::StrategyType::MonteCarlo: {
                    auto eval = std::make_unique<MonteCarloEvaluator>();
                    strategies.push_back(std::make_unique<MonteCarloStrategyV1>(std::move(eval)));
                    names.push_back("MonteCarlo");
                    break;
                }
                case PlayerConfig::StrategyType::GA: {
                    try {
                        Genome g = GenomeSerializer::load(cfg.genomePath.toStdString());
                        strategies.push_back(std::make_unique<GAStrategy>(g));
                        names.push_back(QFileInfo(cfg.genomePath).baseName());
                    } catch (const std::exception& e) {
                        QMessageBox::critical(this, "Load failed", e.what());
                        return;
                    }
                    break;
                }
            }
        }
    }

    mGameWidget = new GameWidget(names, humanIndex, this);
    mStack->addWidget(mGameWidget);
    mStack->setCurrentWidget(mGameWidget);

    mController = new GameController(std::move(strategies), startingChips);
    mGameThread = new QThread(this);
    mController->moveToThread(mGameThread);

    connect(mGameThread,  &QThread::started,              mController,  &GameController::run);
    connect(mController,  &GameController::handComplete,  mGameWidget,  &GameWidget::onHandComplete);
    connect(mController,  &GameController::gameOver,      mGameThread,  &QThread::quit);

    connect(mHumanStrat,  &HumanStrategy::actionRequested, mGameWidget, &GameWidget::onActionRequested);
    connect(mGameWidget,  &GameWidget::actionChosen,        mHumanStrat, &HumanStrategy::provideAction);

    mGameThread->start();
}

void MainWindow::stopGame() {
    if (mController) mController->stop();
    if (mGameThread) {
        mGameThread->quit();
        mGameThread->wait(3000);
        delete mGameThread;
        mGameThread = nullptr;
    }
    if (mGameWidget) {
        mStack->removeWidget(mGameWidget);
        delete mGameWidget;
        mGameWidget = nullptr;
    }
    // mController deleted by thread cleanup (moveToThread transfers ownership)
    mController = nullptr;
    // mHumanStrat owned by strategies vector inside Poker — already deleted above
    mHumanStrat = nullptr;
}

} // namespace poker
