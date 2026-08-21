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
