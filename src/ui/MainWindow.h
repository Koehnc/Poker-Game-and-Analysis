#pragma once
#include "ui/SetupWidget.h"
#include <QMainWindow>
#include <QVector>

class QStackedWidget;
class QThread;

namespace poker {

class GameWidget;
class GameController;
class HumanStrategy;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onGameStartRequested(QVector<poker::PlayerConfig> players, int startingChips);

private:
    void stopGame();

    QStackedWidget* mStack;
    SetupWidget*    mSetupWidget;
    GameWidget*     mGameWidget  = nullptr;
    GameController* mController  = nullptr;
    QThread*        mGameThread  = nullptr;
    HumanStrategy*  mHumanStrat  = nullptr;
};

} // namespace poker
