#pragma once
#include "ui/PokerQtTypes.h"
#include "ui/ActionPanel.h"
#include <QWidget>
#include <QVector>

class QLabel;
class QVBoxLayout;
class QHBoxLayout;

namespace poker {

class GameWidget : public QWidget {
    Q_OBJECT
public:
    explicit GameWidget(QStringList playerNames, int humanIndex, QWidget* parent = nullptr);

public slots:
    void onActionRequested(poker::GameStateView state);
    void onHandComplete(int winner, QVector<int> chipCounts);

signals:
    void actionChosen(poker::Action action);

private:
    QString cardText(const Card& c) const;

    int          mHumanIndex;
    QStringList  mPlayerNames;

    QVector<QLabel*> mChipLabels;
    QLabel*          mBoardLabel;
    QLabel*          mPotLabel;
    QLabel*          mHumanHandLabel;
    QLabel*          mHumanChipsLabel;
    QLabel*          mStatusLabel;
    ActionPanel*     mActionPanel;
};

} // namespace poker
