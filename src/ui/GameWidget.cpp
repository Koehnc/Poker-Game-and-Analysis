#include "ui/GameWidget.h"
#include "core/Card.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFrame>
#include <QListWidget>
#include <QGraphicsDropShadowEffect>
#include <QColor>
#include <utility>
#include <vector>

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

QWidget* GameWidget::makePositionBadges(QWidget* parent) {
    auto* container = new QWidget(parent);
    auto* layout = new QVBoxLayout(container);
    layout->setSpacing(4);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* dealerDot = new QLabel("D", container);
    dealerDot->setObjectName("dealerDot");
    dealerDot->setFixedSize(18, 18);
    dealerDot->setAlignment(Qt::AlignCenter);
    dealerDot->setVisible(false);

    auto* sbDot = new QLabel("SB", container);
    sbDot->setObjectName("sbDot");
    sbDot->setFixedSize(18, 18);
    sbDot->setAlignment(Qt::AlignCenter);
    sbDot->setVisible(false);

    auto* bbDot = new QLabel("BB", container);
    bbDot->setObjectName("bbDot");
    bbDot->setFixedSize(18, 18);
    bbDot->setAlignment(Qt::AlignCenter);
    bbDot->setVisible(false);

    layout->addWidget(dealerDot);
    layout->addWidget(sbDot);
    layout->addWidget(bbDot);
    layout->addStretch();

    return container;
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

bool GameWidget::sameCard(const Card& a, const Card& b) {
    // Card::operator== only compares rank, not suit, so it can't be used
    // to identify a specific card among duplicates-by-rank on the table.
    return a.getRank() == b.getRank() && a.getSuit() == b.getSuit();
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

    // ── Table ring: opponents around an oval, board in the center ──────────
    // A 2x3 grid approximates the oval — top-left/top-center/top-right across
    // the back, left/right flanking the board. Which slots get used depends on
    // how many opponents there are (1-5), keeping each count's arrangement
    // symmetric rather than always filling left-to-right.
    auto* tableGrid = new QGridLayout;
    tableGrid->setHorizontalSpacing(6);
    tableGrid->setVerticalSpacing(2);
    tableGrid->setColumnStretch(0, 1);
    tableGrid->setColumnStretch(1, 3);
    tableGrid->setColumnStretch(2, 1);
    tableGrid->setRowStretch(0, 1);
    tableGrid->setRowStretch(1, 2);

    static const std::vector<std::pair<int, int>> kRingSlots = {
        {0, 0}, {0, 1}, {0, 2}, {1, 0}, {1, 2}   // TL, TC, TR, L, R
    };
    static const std::vector<std::vector<int>> kSlotsByOpponentCount = {
        /*0*/ {},
        /*1*/ {1},
        /*2*/ {0, 2},
        /*3*/ {0, 1, 2},
        /*4*/ {0, 2, 3, 4},
        /*5*/ {0, 1, 2, 3, 4},
    };

    int numOpponents = playerNames.size() - 1;
    const std::vector<int>& slotsForCount =
        (numOpponents >= 0 && numOpponents < (int)kSlotsByOpponentCount.size())
            ? kSlotsByOpponentCount[numOpponents]
            : kSlotsByOpponentCount.back();

    int opponentSlot = 0;
    for (int i = 0; i < playerNames.size(); ++i) {
        if (i == humanIndex) continue;

        auto* seat = new QFrame(tableCol);
        seat->setObjectName("opponentSeat");

        auto* sl = new QVBoxLayout(seat);
        sl->setSpacing(2);
        sl->setContentsMargins(4, 2, 4, 0);

        auto* nameL = new QLabel(playerNames[i], seat);
        nameL->setObjectName("nameLabel");
        nameL->setAlignment(Qt::AlignCenter);

        auto* chipL = new QLabel("$10000", seat);
        chipL->setObjectName("chipLabel");
        chipL->setAlignment(Qt::AlignCenter);

        // Position badges + two face-down cards, side by side
        auto* badge = makePositionBadges(seat);

        auto* cardRow = new QHBoxLayout;
        cardRow->setSpacing(5);
        cardRow->setAlignment(Qt::AlignCenter);
        auto* c1 = makeCardWidget(seat);
        auto* c2 = makeCardWidget(seat);
        setCardBack(c1);
        setCardBack(c2);
        cardRow->addWidget(badge);
        cardRow->addWidget(c1);
        cardRow->addWidget(c2);

        sl->addWidget(chipL);
        sl->addWidget(nameL);
        sl->addLayout(cardRow);

        int slotIdx = (opponentSlot < (int)slotsForCount.size())
            ? slotsForCount[opponentSlot]
            : opponentSlot % (int)kRingSlots.size();
        const auto& cell = kRingSlots[slotIdx];
        tableGrid->addWidget(seat, cell.first, cell.second);
        ++opponentSlot;

        mChipLabels.push_back(chipL);
        mSeatBadges.push_back(badge);
        mOpponentIndices.push_back(i);
        mOpponentCard1.push_back(c1);
        mOpponentCard2.push_back(c2);
    }

    // ── Board (center oval) ────────────────────────────────────────────────
    auto* boardFrame = new QFrame(tableCol);
    boardFrame->setObjectName("boardFrame");
    boardFrame->setMaximumHeight(240);   // stay wide/short so border-radius reads as an oval, not a rounded square

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
    tableGrid->addWidget(boardFrame, 1, 1);

    root->addLayout(tableGrid, 1);

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

    mHumanBadge = makePositionBadges(humanFrame);

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

    // ── Sidebar: history log + outcome bubble (right panel) ─────────────────
    auto* sidebar = new QWidget(this);
    sidebar->setFixedWidth(220);
    auto* sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setSpacing(10);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);

    mHistoryLog = new QListWidget(sidebar);
    mHistoryLog->setObjectName("historyLog");
    mHistoryLog->setSelectionMode(QAbstractItemView::NoSelection);
    mHistoryLog->setFocusPolicy(Qt::NoFocus);
    sidebarLayout->addWidget(mHistoryLog, 3);

    mOutcomeLog = new QListWidget(sidebar);
    mOutcomeLog->setObjectName("outcomeLog");
    mOutcomeLog->setSelectionMode(QAbstractItemView::NoSelection);
    mOutcomeLog->setFocusPolicy(Qt::NoFocus);
    sidebarLayout->addWidget(mOutcomeLog, 1);

    outer->addWidget(sidebar);
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
    int sb = (dealerIndex + 1) % n;
    int bb = (dealerIndex + 2) % n;

    auto applyDots = [dealerIndex, sb, bb](QWidget* container, int playerIndex) {
        container->findChild<QLabel*>("dealerDot")->setVisible(playerIndex == dealerIndex);
        container->findChild<QLabel*>("sbDot")->setVisible(playerIndex == sb);
        container->findChild<QLabel*>("bbDot")->setVisible(playerIndex == bb);
    };

    for (int k = 0; k < mOpponentIndices.size(); ++k) {
        applyDots(mSeatBadges[k], mOpponentIndices[k]);
    }
    applyDots(mHumanBadge, mHumanIndex);
}

void GameWidget::logAction(const ActionRecord& record) {
    QString name = (record.playerId < mPlayerNames.size()) ? mPlayerNames[record.playerId] : "Player";
    bool human = (record.playerId == mHumanIndex);
    QString line;
    switch (record.action) {
        case ActionType::Fold:  line = QString("%1 %2").arg(name, human ? "fold" : "folds"); break;
        case ActionType::Check: line = QString("%1 %2").arg(name, human ? "check" : "checks"); break;
        case ActionType::Call:  line = QString("%1 %2 $%3").arg(name, human ? "call" : "calls").arg(record.amount); break;
        case ActionType::Raise: line = QString("%1 %2 $%3").arg(name, human ? "raise" : "raises").arg(record.amount); break;
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

QFrame* GameWidget::opponentCardWidget(int playerId, int cardIndex) {
    int idx = mOpponentIndices.indexOf(playerId);
    if (idx < 0) return nullptr;
    return cardIndex == 0 ? mOpponentCard1[idx] : mOpponentCard2[idx];
}

void GameWidget::setActingGlow(int playerId) {
    QFrame* c1 = nullptr;
    QFrame* c2 = nullptr;
    if (playerId == mHumanIndex) {
        if (mHumanCards.size() >= 2) { c1 = mHumanCards[0]; c2 = mHumanCards[1]; }
    } else {
        c1 = opponentCardWidget(playerId, 0);
        c2 = opponentCardWidget(playerId, 1);
    }
    if (!c1 || !c2) { clearGlow(); return; }
    applyGlow({c1, c2});
}

void GameWidget::applyGlow(const QVector<QFrame*>& frames) {
    clearGlow();
    for (auto* frame : frames) {
        auto* effect = new QGraphicsDropShadowEffect;
        effect->setColor(QColor("#c4991f"));
        effect->setBlurRadius(24);
        effect->setOffset(0, 0);
        frame->setGraphicsEffect(effect);
    }
    mGlowFrames = frames;
}

void GameWidget::clearGlow() {
    for (auto* frame : mGlowFrames) frame->setGraphicsEffect(nullptr);
    mGlowFrames.clear();
}

void GameWidget::clearPlayerCards(int playerId) {
    if (playerId == mHumanIndex) {
        for (auto* card : mHumanCards) clearCard(card);
    } else {
        QFrame* c1 = opponentCardWidget(playerId, 0);
        QFrame* c2 = opponentCardWidget(playerId, 1);
        if (c1) clearCard(c1);
        if (c2) clearCard(c2);
    }
}

void GameWidget::resetTable() {
    clearGlow();
    for (auto* card : mBoardCards) clearCard(card);
    for (auto* card : mHumanCards) setCardBack(card);
    for (auto* card : mOpponentCard1) setCardBack(card);
    for (auto* card : mOpponentCard2) setCardBack(card);
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
    setActingGlow(mHumanIndex);
    mActionPanel->activate(state.minToCall, state.pot, state.playerStack);
}

void GameWidget::onStepOccurred(poker::HandStep step) {
    refreshBoard(step.pot, step.board);
    updateBadges(step.dealerIndex);
    mLastPot = step.pot;
    mLastBoard = step.board;

    switch (step.kind) {
        case StepKind::HandStarted:
            mHistoryLog->clear();
            mHistoryLog->addItem("— New hand —");
            resetTable();
            break;
        case StepKind::StreetDealt:
            logStreet(step.round, step.board);
            clearGlow();
            break;
        case StepKind::PlayerToAct: {
            QString actorName = (step.action.playerId < mPlayerNames.size()) ? mPlayerNames[step.action.playerId] : "Player";
            mStatusLabel->setText(QString("%1 acting...").arg(actorName));
            setActingGlow(step.action.playerId);
            break;
        }
        case StepKind::PlayerActed:
            logAction(step.action);
            clearGlow();
            if (step.action.action == ActionType::Fold) {
                clearPlayerCards(step.action.playerId);
            }
            break;
    }
}

void GameWidget::onHandComplete(int winner, QVector<int> chipCounts,
                                 std::vector<poker::Card> winnerHoleCards,
                                 std::vector<poker::Card> winningCards) {
    mActionPanel->deactivate();
    clearGlow();

    QString name = (winner < mPlayerNames.size()) ? mPlayerNames[winner] : "Unknown";
    QString winVerb = (winner == mHumanIndex) ? "win" : "wins";
    mStatusLabel->setText(QString("%1 %2 the hand!").arg(name, winVerb));
    mOutcomeLog->insertItem(0, QString("%1 %2 $%3").arg(name, winVerb).arg(mLastPot));

    // Update chip labels
    mHumanChipsLabel->setText(
        QString("You (%1)   $%2").arg(mPlayerNames[mHumanIndex]).arg(chipCounts[mHumanIndex]));

    for (int k = 0; k < mOpponentIndices.size(); ++k) {
        mChipLabels[k]->setText(QString("$%1").arg(chipCounts[mOpponentIndices[k]]));
    }

    // The board/cards are intentionally left as-is here (not cleared) so the
    // reveal below stays visible through the pause — resetTable() runs on the
    // next hand's HandStarted step instead.

    // winningCards is only non-empty for a genuine showdown; an uncontested
    // fold-around win reveals nothing, same as real poker.
    if (winnerHoleCards.size() < 2 || winningCards.empty()) return;

    if (winner != mHumanIndex) {
        QFrame* c1 = opponentCardWidget(winner, 0);
        QFrame* c2 = opponentCardWidget(winner, 1);
        if (c1) setCardFace(c1, winnerHoleCards[0]);
        if (c2) setCardFace(c2, winnerHoleCards[1]);
    }

    QVector<QFrame*> winFrames;
    for (const Card& wc : winningCards) {
        bool matched = false;
        for (int i = 0; i < static_cast<int>(mLastBoard.size()) && i < mBoardCards.size(); ++i) {
            if (sameCard(wc, mLastBoard[i])) {
                winFrames.push_back(mBoardCards[i]);
                matched = true;
                break;
            }
        }
        if (matched) continue;

        for (int i = 0; i < static_cast<int>(winnerHoleCards.size()); ++i) {
            if (sameCard(wc, winnerHoleCards[i])) {
                QFrame* holeFrame = (winner == mHumanIndex)
                    ? (i < mHumanCards.size() ? mHumanCards[i] : nullptr)
                    : opponentCardWidget(winner, i);
                if (holeFrame) winFrames.push_back(holeFrame);
                break;
            }
        }
    }
    applyGlow(winFrames);
}

} // namespace poker
