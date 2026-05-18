#include "ui/ActionPanel.h"
#include "game/VisibleInformation.h"
#include <QPushButton>
#include <QSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <algorithm>

namespace poker {

ActionPanel::ActionPanel(QWidget* parent) : QWidget(parent) {
    mFoldBtn  = new QPushButton("Fold",  this);
    mCallBtn  = new QPushButton("Call",  this);
    mRaiseBtn = new QPushButton("Raise", this);
    mBetBox   = new QSpinBox(this);
    mBetBox->setMinimum(1);
    mBetBox->setMaximum(1'000'000);

    auto* layout = new QHBoxLayout(this);
    layout->addWidget(mFoldBtn);
    layout->addWidget(mCallBtn);
    layout->addWidget(mRaiseBtn);
    layout->addWidget(new QLabel("Bet:", this));
    layout->addWidget(mBetBox);

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

    mCallBtn->setText(minToCall > 0 ? QString("Call $%1").arg(minToCall) : "Check");

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
