#include "ui/GameWidget.h"
#include "core/Card.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>

namespace poker {

GameWidget::GameWidget(QStringList playerNames, int humanIndex, QWidget* parent)
    : QWidget(parent), mHumanIndex(humanIndex), mPlayerNames(playerNames)
{
    auto* root = new QVBoxLayout(this);
    root->setSpacing(12);

    // Opponent seats (top)
    auto* opponentRow = new QHBoxLayout;
    for (int i = 0; i < playerNames.size(); ++i) {
        if (i == humanIndex) continue;
        auto* seat = new QFrame(this);
        seat->setFrameShape(QFrame::Box);
        seat->setStyleSheet("QFrame { background: #2a3a2a; border-radius: 8px; padding: 6px; }");
        auto* sl = new QVBoxLayout(seat);
        auto* nameL  = new QLabel(playerNames[i], seat);
        auto* chipL  = new QLabel("$10000", seat);
        auto* cardsL = new QLabel("🂠 🂠", seat);
        nameL->setAlignment(Qt::AlignCenter);
        chipL->setAlignment(Qt::AlignCenter);
        cardsL->setAlignment(Qt::AlignCenter);
        cardsL->setStyleSheet("font-size: 20px;");
        sl->addWidget(nameL);
        sl->addWidget(chipL);
        sl->addWidget(cardsL);
        opponentRow->addWidget(seat);
        mChipLabels.push_back(chipL);
    }
    root->addLayout(opponentRow);

    // Board (center)
    auto* boardFrame = new QFrame(this);
    boardFrame->setStyleSheet("QFrame { background: #155221; border-radius: 40px; padding: 12px; }");
    auto* boardLayout = new QVBoxLayout(boardFrame);
    mPotLabel   = new QLabel("Pot: $0", boardFrame);
    mBoardLabel = new QLabel("Waiting for deal...", boardFrame);
    mPotLabel->setAlignment(Qt::AlignCenter);
    mBoardLabel->setAlignment(Qt::AlignCenter);
    mBoardLabel->setStyleSheet("font-size: 28px; letter-spacing: 8px;");
    boardLayout->addWidget(mPotLabel);
    boardLayout->addWidget(mBoardLabel);
    root->addWidget(boardFrame);

    // Human seat (bottom)
    auto* humanFrame = new QFrame(this);
    humanFrame->setStyleSheet("QFrame { background: #1e3a2e; border: 2px solid #27ae60; border-radius: 8px; padding: 8px; }");
    auto* humanLayout = new QVBoxLayout(humanFrame);
    mHumanHandLabel  = new QLabel("🂠 🂠", humanFrame);
    mHumanChipsLabel = new QLabel(QString("You (%1)  $10000").arg(playerNames[humanIndex]), humanFrame);
    mHumanHandLabel->setAlignment(Qt::AlignCenter);
    mHumanHandLabel->setStyleSheet("font-size: 32px;");
    mHumanChipsLabel->setAlignment(Qt::AlignCenter);
    humanLayout->addWidget(mHumanHandLabel);
    humanLayout->addWidget(mHumanChipsLabel);
    root->addWidget(humanFrame);

    // Status line
    mStatusLabel = new QLabel("Waiting...", this);
    mStatusLabel->setAlignment(Qt::AlignCenter);
    root->addWidget(mStatusLabel);

    // Action panel
    mActionPanel = new ActionPanel(this);
    root->addWidget(mActionPanel);

    connect(mActionPanel, &ActionPanel::actionChosen, this, &GameWidget::actionChosen);
}

QString GameWidget::cardText(const Card& c) const {
    QString rank;
    switch (c.getRank()) {
        case 14: rank = "A"; break;
        case 13: rank = "K"; break;
        case 12: rank = "Q"; break;
        case 11: rank = "J"; break;
        default: rank = QString::number(c.getRank());
    }
    QString suit;
    switch (c.getSuit()) {
        case 0: suit = "♣"; break;
        case 1: suit = "♦"; break;
        case 2: suit = "♥"; break;
        case 3: suit = "♠"; break;
        default: suit = "?";
    }
    return rank + suit;
}

void GameWidget::onActionRequested(poker::GameStateView state) {
    QString boardStr;
    for (const auto& c : state.board) boardStr += cardText(c) + " ";
    mBoardLabel->setText(boardStr.isEmpty() ? "(no board)" : boardStr.trimmed());
    mPotLabel->setText(QString("Pot: $%1").arg(state.pot));

    QString handStr;
    for (const auto& c : state.playerHand) handStr += cardText(c) + " ";
    mHumanHandLabel->setText(handStr.trimmed());

    mStatusLabel->setText("Your turn");
    mActionPanel->activate(state.minToCall, state.pot, state.playerStack);
}

void GameWidget::onHandComplete(int winner, QVector<int> chipCounts) {
    mActionPanel->deactivate();
    QString name = (winner < mPlayerNames.size()) ? mPlayerNames[winner] : "Unknown";
    mStatusLabel->setText(QString("Hand over — %1 wins!").arg(name));

    int opponentIdx = 0;
    for (int i = 0; i < mPlayerNames.size(); ++i) {
        if (i == mHumanIndex) {
            mHumanChipsLabel->setText(
                QString("You (%1)  $%2").arg(mPlayerNames[i]).arg(chipCounts[i]));
        } else if (opponentIdx < mChipLabels.size()) {
            mChipLabels[opponentIdx++]->setText(QString("$%1").arg(chipCounts[i]));
        }
    }
}

} // namespace poker
