#include "ui/ActionPanel.h"
#include "game/VisibleInformation.h"
#include <QPushButton>
#include <QSpinBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <algorithm>

namespace poker {

ActionPanel::ActionPanel(QWidget* parent) : QWidget(parent) {
    mFoldBtn  = new QPushButton("FOLD",  this);
    mCallBtn  = new QPushButton("CHECK", this);
    mRaiseBtn = new QPushButton("RAISE", this);

    mFoldBtn->setObjectName("foldBtn");
    mCallBtn->setObjectName("callBtn");
    mRaiseBtn->setObjectName("raiseBtn");

    mBetBox = new QSpinBox(this);
    mBetBox->setMinimum(1);
    mBetBox->setMaximum(1'000'000);
    mBetBox->setFixedWidth(110);

    auto* betLabel = new QLabel("BET", this);
    betLabel->setObjectName("sectionLabel");

    // Raise sub-row: button + label + spinbox
    auto* raiseRow = new QHBoxLayout;
    raiseRow->setSpacing(8);
    raiseRow->setContentsMargins(0, 0, 0, 0);
    raiseRow->addWidget(mRaiseBtn, 1);
    raiseRow->addWidget(betLabel);
    raiseRow->addWidget(mBetBox);

    auto* layout = new QHBoxLayout(this);
    layout->setSpacing(12);
    layout->setContentsMargins(0, 6, 0, 6);
    layout->addWidget(mFoldBtn, 1);
    layout->addWidget(mCallBtn, 1);
    layout->addLayout(raiseRow, 2);

    connect(mFoldBtn,  &QPushButton::clicked, this, &ActionPanel::onFold);
    connect(mCallBtn,  &QPushButton::clicked, this, &ActionPanel::onCall);
    connect(mRaiseBtn, &QPushButton::clicked, this, &ActionPanel::onRaise);

    deactivate();
}

void ActionPanel::activate(int minToCall, int pot, int playerStack) {
    mMinToCall = minToCall;
    int defaultRaise = std::min(playerStack, minToCall * 2 + pot / 2);
    mBetBox->setValue(std::max(minToCall * 2, defaultRaise));
    mBetBox->setMaximum(playerStack);

    mCallBtn->setText(minToCall > 0
        ? QString("CALL  $%1").arg(minToCall)
        : "CHECK");

    setEnabled(true);
}

void ActionPanel::deactivate() {
    setEnabled(false);
}

void ActionPanel::onFold() {
    deactivate();
    emit actionChosen(Action{ActionType::Fold, 0});
}

void ActionPanel::onCall() {
    deactivate();
    if (mMinToCall == 0)
        emit actionChosen(Action{ActionType::Check, 0});
    else
        emit actionChosen(Action{ActionType::Call, mMinToCall});
}

void ActionPanel::onRaise() {
    deactivate();
    emit actionChosen(Action{ActionType::Raise, mBetBox->value()});
}

} // namespace poker
