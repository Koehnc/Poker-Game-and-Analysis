#include "ui/GameWidget.h"
#include "core/Card.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QListWidget>

namespace poker {

// ─── Card widget helpers ───────────────────────────────────────────────────

QFrame* GameWidget::makeCardWidget(QWidget* parent) {
    auto* frame = new QFrame(parent);
    frame->setFixedSize(52, 74);
    auto* label = new QLabel("", frame);
    label->setObjectName("cardLabel");
    label->setGeometry(0, 0, 52, 74);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("background: transparent;");
    return frame;
}

QString GameWidget::cardShortText(const Card& c) {
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

void GameWidget::setCardFace(QFrame* card, const Card& c) {
    QString rank;
    switch (c.getRank()) {
        case 14: rank = "A"; break;
        case 13: rank = "K"; break;
        case 12: rank = "Q"; break;
        case 11: rank = "J"; break;
        default: rank = QString::number(c.getRank());
    }

    QString suit, color;
    switch (c.getSuit()) {
        case 0: suit = "♣"; color = "#1a1a1a"; break;  // clubs
        case 1: suit = "♦"; color = "#c0202a"; break;  // diamonds
        case 2: suit = "♥"; color = "#c0202a"; break;  // hearts
        case 3: suit = "♠"; color = "#1a1a1a"; break;  // spades
        default: suit = "?";     color = "#555";    break;
    }

    card->setStyleSheet(
        "QFrame { background-color: #f5f0e8; border: 1px solid #c8c0b0; border-radius: 8px; }"
    );

    auto* label = card->findChild<QLabel*>("cardLabel");
    if (!label) return;
    label->setStyleSheet(QString(
        "background: transparent;"
        "color: %1;"
        "font-family: Georgia;"
        "font-weight: bold;"
    ).arg(color));
    label->setText(QString(
        "<div style='font-size:15px; line-height:1;'>%1</div>"
        "<div style='font-size:21px; line-height:1;'>%2</div>"
    ).arg(rank, suit));
    label->setTextFormat(Qt::RichText);
}

void GameWidget::setCardBack(QFrame* card) {
    card->setStyleSheet(
        "QFrame { background-color: #1a2a5a; border: 1px solid #2a3a80; border-radius: 8px; }"
    );
    auto* label = card->findChild<QLabel*>("cardLabel");
    if (!label) return;
    label->setStyleSheet("background: transparent; color: #2a3a70; font-size: 26px;");
    label->setText("★");
    label->setTextFormat(Qt::PlainText);
}

void GameWidget::clearCard(QFrame* card) {
    card->setStyleSheet(
        "QFrame { background-color: transparent; border: 1px dashed #1a2a1a; border-radius: 8px; }"
    );
    auto* label = card->findChild<QLabel*>("cardLabel");
    if (label) { label->setText(""); }
}

// ─── Constructor ───────────────────────────────────────────────────────────

GameWidget::GameWidget(QStringList playerNames, int humanIndex, QWidget* parent)
    : QWidget(parent), mHumanIndex(humanIndex), mPlayerNames(playerNames)
{
    auto* outer = new QHBoxLayout(this);
    outer->setSpacing(14);
    outer->setContentsMargins(20, 16, 20, 14);

    auto* tableCol = new QWidget(this);
    auto* root = new QVBoxLayout(tableCol);
    root->setSpacing(10);
    root->setContentsMargins(0, 0, 0, 0);

    // ── Opponent seats (top row) ───────────────────────────────────────────
    auto* opponentRow = new QHBoxLayout;
    opponentRow->setSpacing(12);

    for (int i = 0; i < playerNames.size(); ++i) {
        if (i == humanIndex) continue;

        auto* seat = new QFrame(tableCol);
        seat->setObjectName("opponentSeat");

        auto* sl = new QVBoxLayout(seat);
        sl->setSpacing(5);
        sl->setContentsMargins(14, 12, 14, 12);

        auto* badge = new QLabel("", seat);
        badge->setObjectName("seatBadge");
        badge->setAlignment(Qt::AlignCenter);
        badge->setVisible(false);

        auto* nameL = new QLabel(playerNames[i], seat);
        nameL->setObjectName("nameLabel");
        nameL->setAlignment(Qt::AlignCenter);

        auto* chipL = new QLabel("$10000", seat);
        chipL->setObjectName("chipLabel");
        chipL->setAlignment(Qt::AlignCenter);

        // Two face-down cards
        auto* cardRow = new QHBoxLayout;
        cardRow->setSpacing(5);
        cardRow->setAlignment(Qt::AlignCenter);
        auto* c1 = makeCardWidget(seat);
        auto* c2 = makeCardWidget(seat);
        setCardBack(c1);
        setCardBack(c2);
        cardRow->addWidget(c1);
        cardRow->addWidget(c2);

        sl->addWidget(badge);
        sl->addWidget(nameL);
        sl->addWidget(chipL);
        sl->addLayout(cardRow);

        opponentRow->addWidget(seat);
        mChipLabels.push_back(chipL);
        mSeatBadges.push_back(badge);
        mOpponentIndices.push_back(i);
    }
    root->addLayout(opponentRow);

    // ── Board (center oval) ────────────────────────────────────────────────
    auto* boardFrame = new QFrame(tableCol);
    boardFrame->setObjectName("boardFrame");

    auto* boardLayout = new QVBoxLayout(boardFrame);
    boardLayout->setSpacing(10);
    boardLayout->setContentsMargins(28, 18, 28, 18);

    mPotLabel = new QLabel("POT  —  $0", boardFrame);
    mPotLabel->setObjectName("potLabel");
    mPotLabel->setAlignment(Qt::AlignCenter);

    auto* cardRowLayout = new QHBoxLayout;
    cardRowLayout->setSpacing(10);
    cardRowLayout->setAlignment(Qt::AlignCenter);
    for (int i = 0; i < 5; ++i) {
        auto* card = makeCardWidget(boardFrame);
        clearCard(card);
        mBoardCards.push_back(card);
        cardRowLayout->addWidget(card);
    }

    boardLayout->addWidget(mPotLabel);
    boardLayout->addLayout(cardRowLayout);
    root->addWidget(boardFrame, 1);

    // ── Status bar ─────────────────────────────────────────────────────────
    auto* statusBar = new QFrame(tableCol);
    statusBar->setObjectName("statusBar");
    auto* statusLayout = new QHBoxLayout(statusBar);
    statusLayout->setContentsMargins(0, 0, 0, 0);
    mStatusLabel = new QLabel("Waiting for hand to begin...", statusBar);
    mStatusLabel->setObjectName("statusLabel");
    mStatusLabel->setAlignment(Qt::AlignCenter);
    statusLayout->addWidget(mStatusLabel);
    root->addWidget(statusBar);

    // ── Human seat (bottom) ────────────────────────────────────────────────
    auto* humanFrame = new QFrame(tableCol);
    humanFrame->setObjectName("humanSeat");

    auto* humanLayout = new QHBoxLayout(humanFrame);
    humanLayout->setSpacing(18);
    humanLayout->setContentsMargins(20, 14, 20, 14);

    mHumanBadge = new QLabel("", humanFrame);
    mHumanBadge->setObjectName("seatBadge");
    mHumanBadge->setAlignment(Qt::AlignCenter);
    mHumanBadge->setVisible(false);

    auto* handRow = new QHBoxLayout;
    handRow->setSpacing(8);
    handRow->setAlignment(Qt::AlignCenter);
    for (int i = 0; i < 2; ++i) {
        auto* card = makeCardWidget(humanFrame);
        setCardBack(card);
        mHumanCards.push_back(card);
        handRow->addWidget(card);
    }

    mHumanChipsLabel = new QLabel(
        QString("You (%1)   $10000").arg(playerNames[humanIndex]),
        humanFrame);
    mHumanChipsLabel->setObjectName("chipLabel");
    mHumanChipsLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    humanLayout->addWidget(mHumanBadge);
    humanLayout->addLayout(handRow);
    humanLayout->addWidget(mHumanChipsLabel, 1);
    root->addWidget(humanFrame);

    // ── Action panel ───────────────────────────────────────────────────────
    mActionPanel = new ActionPanel(tableCol);
    root->addWidget(mActionPanel);

    connect(mActionPanel, &ActionPanel::actionChosen, this, &GameWidget::actionChosen);

    outer->addWidget(tableCol, 1);

    // ── History log (right panel) ──────────────────────────────────────────
    mHistoryLog = new QListWidget(this);
    mHistoryLog->setObjectName("historyLog");
    mHistoryLog->setFixedWidth(220);
    outer->addWidget(mHistoryLog);
}

// ─── Shared update helpers ──────────────────────────────────────────────────

void GameWidget::refreshBoard(int pot, const std::vector<Card>& board) {
    mPotLabel->setText(QString("POT  —  $%1").arg(pot));
    for (int i = 0; i < 5; ++i) {
        if (i < static_cast<int>(board.size()))
            setCardFace(mBoardCards[i], board[i]);
        else
            clearCard(mBoardCards[i]);
    }
}

void GameWidget::updateBadges(int dealerIndex) {
    int n = mPlayerNames.size();
    auto badgeFor = [dealerIndex, n](int playerIndex) -> QString {
        int offset = ((playerIndex - dealerIndex) % n + n) % n;
        if (offset == 0) return "D";
        if (offset == 1) return "SB";
        if (offset == 2) return "BB";
        return "";
    };

    for (int k = 0; k < mOpponentIndices.size(); ++k) {
        QString text = badgeFor(mOpponentIndices[k]);
        mSeatBadges[k]->setText(text);
        mSeatBadges[k]->setVisible(!text.isEmpty());
    }

    QString humanText = badgeFor(mHumanIndex);
    mHumanBadge->setText(humanText);
    mHumanBadge->setVisible(!humanText.isEmpty());
}

void GameWidget::logAction(const ActionRecord& record) {
    QString name = (record.playerId < mPlayerNames.size()) ? mPlayerNames[record.playerId] : "Player";
    QString line;
    switch (record.action) {
        case ActionType::Fold:  line = QString("%1 folds").arg(name); break;
        case ActionType::Check: line = QString("%1 checks").arg(name); break;
        case ActionType::Call:  line = QString("%1 calls $%2").arg(name).arg(record.amount); break;
        case ActionType::Raise: line = QString("%1 raises to $%2").arg(name).arg(record.amount); break;
    }
    mHistoryLog->addItem(line);
    mHistoryLog->scrollToBottom();
}

void GameWidget::logStreet(Round round, const std::vector<Card>& board) {
    QString label;
    int newCardCount = 0;
    switch (round) {
        case Round::Flop:  label = "Flop";  newCardCount = 3; break;
        case Round::Turn:  label = "Turn";  newCardCount = 1; break;
        case Round::River: label = "River"; newCardCount = 1; break;
        default: return;
    }
    QStringList cardStrs;
    int start = static_cast<int>(board.size()) - newCardCount;
    for (int i = start; i < static_cast<int>(board.size()); ++i)
        cardStrs << cardShortText(board[i]);
    mHistoryLog->addItem(QString("%1: %2").arg(label, cardStrs.join(' ')));
    mHistoryLog->scrollToBottom();
}

// ─── Slots ─────────────────────────────────────────────────────────────────

void GameWidget::onActionRequested(poker::GameStateView state) {
    refreshBoard(state.pot, state.board);
    updateBadges(state.dealerIndex);

    // Hole cards
    for (int i = 0; i < 2; ++i) {
        if (i < static_cast<int>(state.playerHand.size()))
            setCardFace(mHumanCards[i], state.playerHand[i]);
        else
            setCardBack(mHumanCards[i]);
    }

    mStatusLabel->setText("Your turn");
    mActionPanel->activate(state.minToCall, state.pot, state.playerStack);
}

void GameWidget::onStepOccurred(poker::HandStep step) {
    refreshBoard(step.pot, step.board);
    updateBadges(step.dealerIndex);

    switch (step.kind) {
        case StepKind::HandStarted:
            mHistoryLog->clear();
            mHistoryLog->addItem("— New hand —");
            break;
        case StepKind::StreetDealt:
            logStreet(step.round, step.board);
            break;
        case StepKind::PlayerActed:
            logAction(step.action);
            break;
    }
}

void GameWidget::onHandComplete(int winner, QVector<int> chipCounts) {
    mActionPanel->deactivate();

    QString name = (winner < mPlayerNames.size()) ? mPlayerNames[winner] : "Unknown";
    mStatusLabel->setText(QString("%1 wins the hand!").arg(name));

    // Reset board to empty
    for (auto* card : mBoardCards) clearCard(card);
    for (auto* card : mHumanCards) setCardBack(card);

    // Update chip labels
    int opponentIdx = 0;
    for (int i = 0; i < mPlayerNames.size(); ++i) {
        if (i == mHumanIndex) {
            mHumanChipsLabel->setText(
                QString("You (%1)   $%2").arg(mPlayerNames[i]).arg(chipCounts[i]));
        } else if (opponentIdx < mChipLabels.size()) {
            mChipLabels[opponentIdx++]->setText(QString("$%1").arg(chipCounts[i]));
        }
    }
}

} // namespace poker
