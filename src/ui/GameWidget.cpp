#include "ui/GameWidget.h"
#include "core/Card.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>

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

void GameWidget::setCardFace(QFrame* card, const Card& c) {
    // Rank text
    QString rank;
    switch (c.getRank()) {
        case 14: rank = "A"; break;
        case 13: rank = "K"; break;
        case 12: rank = "Q"; break;
        case 11: rank = "J"; break;
        default: rank = QString::number(c.getRank());
    }

    // Suit text and color
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
    auto* root = new QVBoxLayout(this);
    root->setSpacing(10);
    root->setContentsMargins(20, 16, 20, 14);

    // ── Opponent seats (top row) ───────────────────────────────────────────
    auto* opponentRow = new QHBoxLayout;
    opponentRow->setSpacing(12);

    for (int i = 0; i < playerNames.size(); ++i) {
        if (i == humanIndex) continue;

        auto* seat = new QFrame(this);
        seat->setObjectName("opponentSeat");

        auto* sl = new QVBoxLayout(seat);
        sl->setSpacing(5);
        sl->setContentsMargins(14, 12, 14, 12);

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

        sl->addWidget(nameL);
        sl->addWidget(chipL);
        sl->addLayout(cardRow);

        opponentRow->addWidget(seat);
        mChipLabels.push_back(chipL);
    }
    root->addLayout(opponentRow);

    // ── Board (center oval) ────────────────────────────────────────────────
    auto* boardFrame = new QFrame(this);
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
    auto* statusBar = new QFrame(this);
    statusBar->setObjectName("statusBar");
    auto* statusLayout = new QHBoxLayout(statusBar);
    statusLayout->setContentsMargins(0, 0, 0, 0);
    mStatusLabel = new QLabel("Waiting for hand to begin...", statusBar);
    mStatusLabel->setObjectName("statusLabel");
    mStatusLabel->setAlignment(Qt::AlignCenter);
    statusLayout->addWidget(mStatusLabel);
    root->addWidget(statusBar);

    // ── Human seat (bottom) ────────────────────────────────────────────────
    auto* humanFrame = new QFrame(this);
    humanFrame->setObjectName("humanSeat");

    auto* humanLayout = new QHBoxLayout(humanFrame);
    humanLayout->setSpacing(18);
    humanLayout->setContentsMargins(20, 14, 20, 14);

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

    humanLayout->addLayout(handRow);
    humanLayout->addWidget(mHumanChipsLabel, 1);
    root->addWidget(humanFrame);

    // ── Action panel ───────────────────────────────────────────────────────
    mActionPanel = new ActionPanel(this);
    root->addWidget(mActionPanel);

    connect(mActionPanel, &ActionPanel::actionChosen, this, &GameWidget::actionChosen);
}

// ─── Slots ─────────────────────────────────────────────────────────────────

void GameWidget::onActionRequested(poker::GameStateView state) {
    mPotLabel->setText(QString("POT  —  $%1").arg(state.pot));

    // Community cards
    for (int i = 0; i < 5; ++i) {
        if (i < static_cast<int>(state.board.size()))
            setCardFace(mBoardCards[i], state.board[i]);
        else
            clearCard(mBoardCards[i]);
    }

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
