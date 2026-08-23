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
    void onHandComplete(int winner, QVector<int> chipCounts,
                         std::vector<poker::Card> winnerHoleCards,
                         std::vector<poker::Card> winningCards);

signals:
    void actionChosen(poker::Action action);

private:
    static QFrame* makeCardWidget(QWidget* parent);
    static QWidget* makePositionBadges(QWidget* parent);
    void setCardFace(QFrame* card, const Card& c);
    void setCardBack(QFrame* card);
    void clearCard(QFrame* card);
    void refreshBoard(int pot, const std::vector<Card>& board);
    void updateBadges(int dealerIndex);
    void logAction(const ActionRecord& record);
    void logStreet(Round round, const std::vector<Card>& board);
    static QString cardShortText(const Card& c);
    static bool sameCard(const Card& a, const Card& b);
    void setActingGlow(int playerId);
    void applyGlow(const QVector<QFrame*>& frames);
    void clearGlow();
    void resetTable();
    QFrame* opponentCardWidget(int playerId, int cardIndex);
    void clearPlayerCards(int playerId);

    int          mHumanIndex;
    QStringList  mPlayerNames;
    QVector<int> mOpponentIndices;   // actual player index for each entry in mChipLabels/mSeatBadges/mOpponentCard1/mOpponentCard2

    QVector<QLabel*>  mChipLabels;       // one per opponent, in order
    QVector<QWidget*> mSeatBadges;       // one per opponent, parallel to mChipLabels; each holds dealer/SB/BB dots
    QWidget*          mHumanBadge   = nullptr;
    QVector<QFrame*>  mBoardCards;       // 5 community card slots
    QVector<QFrame*>  mHumanCards;       // 2 hole card slots
    QVector<QFrame*>  mOpponentCard1;    // one per opponent, parallel to mOpponentIndices
    QVector<QFrame*>  mOpponentCard2;    // one per opponent, parallel to mOpponentIndices
    QVector<QFrame*>  mGlowFrames;       // currently-glowing card widgets, if any
    QLabel*           mPotLabel      = nullptr;
    QLabel*           mHumanChipsLabel = nullptr;
    QLabel*           mStatusLabel   = nullptr;
    ActionPanel*      mActionPanel   = nullptr;
    QListWidget*      mHistoryLog    = nullptr;
    QListWidget*      mOutcomeLog    = nullptr;
    int               mLastPot       = 0;   // most recent step's pot, used as the amount won on hand completion
    std::vector<Card> mLastBoard;           // most recent step's board, used to match winningCards to board widgets
};

} // namespace poker
