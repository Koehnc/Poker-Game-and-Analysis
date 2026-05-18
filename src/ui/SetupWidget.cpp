#include "ui/SetupWidget.h"
#include "ga/GenomeSerializer.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QGroupBox>

namespace poker {

SetupWidget::SetupWidget(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setSpacing(10);

    auto* title = new QLabel("Configure Table", this);
    title->setStyleSheet("font-size: 18px; font-weight: bold;");
    root->addWidget(title);

    auto* slotsGroup = new QGroupBox("Players", this);
    mSlotsLayout = new QVBoxLayout(slotsGroup);
    root->addWidget(slotsGroup);

    auto* slotBtnRow = new QHBoxLayout;
    mAddBtn    = new QPushButton("+ Add player", this);
    mRemoveBtn = new QPushButton("- Remove last", this);
    slotBtnRow->addWidget(mAddBtn);
    slotBtnRow->addWidget(mRemoveBtn);
    root->addLayout(slotBtnRow);

    auto* chipsRow = new QHBoxLayout;
    chipsRow->addWidget(new QLabel("Starting chips:", this));
    mChipsBox = new QSpinBox(this);
    mChipsBox->setRange(100, 1'000'000);
    mChipsBox->setValue(10000);
    mChipsBox->setSingleStep(1000);
    chipsRow->addWidget(mChipsBox);
    root->addLayout(chipsRow);

    mStartBtn = new QPushButton("Start Game", this);
    mStartBtn->setStyleSheet("QPushButton { background: #27ae60; color: white; padding: 8px; font-size: 14px; border-radius: 4px; }");
    root->addWidget(mStartBtn);

    connect(mAddBtn,    &QPushButton::clicked, this, &SetupWidget::addPlayerSlot);
    connect(mRemoveBtn, &QPushButton::clicked, this, &SetupWidget::removeLastSlot);
    connect(mStartBtn,  &QPushButton::clicked, this, &SetupWidget::onStartClicked);

    // Default: human + 1 GA opponent
    PlayerConfig human; human.isHuman = true;
    mConfigs.push_back(human);
    buildSlotRow(0);

    PlayerConfig ai; ai.strategy = PlayerConfig::StrategyType::GA;
    mConfigs.push_back(ai);
    buildSlotRow(1);
}

void SetupWidget::buildSlotRow(int index) {
    auto* row    = new QWidget(this);
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);

    bool isHuman = mConfigs[index].isHuman;
    auto* label  = new QLabel(isHuman ? "You (Human)" : QString("Seat %1").arg(index + 1), row);
    label->setMinimumWidth(100);
    layout->addWidget(label);

    if (!isHuman) {
        auto* combo = new QComboBox(row);
        combo->addItem("Random",    static_cast<int>(PlayerConfig::StrategyType::Random));
        combo->addItem("MonteCarlo",static_cast<int>(PlayerConfig::StrategyType::MonteCarlo));
        combo->addItem("GA",        static_cast<int>(PlayerConfig::StrategyType::GA));
        combo->setCurrentIndex(2); // default GA
        layout->addWidget(combo);

        auto* fileBtn   = new QPushButton("Load genome...", row);
        auto* fileLabel = new QLabel("(none)", row);
        fileLabel->setStyleSheet("color: #888;");
        layout->addWidget(fileBtn);
        layout->addWidget(fileLabel);

        connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [fileBtn, fileLabel, combo](int) {
                bool isGA = combo->currentData().toInt() == static_cast<int>(PlayerConfig::StrategyType::GA);
                fileBtn->setVisible(isGA);
                fileLabel->setVisible(isGA);
            });

        connect(fileBtn, &QPushButton::clicked, this, [this, index, fileLabel]() {
            QString path = QFileDialog::getOpenFileName(this, "Load Genome", "", "Genome files (*.genome)");
            if (!path.isEmpty()) {
                mConfigs[index].genomePath = path;
                fileLabel->setText(QFileInfo(path).fileName());
                fileLabel->setStyleSheet("color: #27ae60;");
            }
        });

        connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this, index, combo](int) {
                mConfigs[index].strategy = static_cast<PlayerConfig::StrategyType>(combo->currentData().toInt());
            });
    }

    mSlotsLayout->addWidget(row);
    mSlotWidgets.push_back(row);
}

void SetupWidget::addPlayerSlot() {
    if (mConfigs.size() >= 6) return;
    PlayerConfig cfg;
    cfg.strategy = PlayerConfig::StrategyType::MonteCarlo;
    mConfigs.push_back(cfg);
    buildSlotRow(mConfigs.size() - 1);
}

void SetupWidget::removeLastSlot() {
    if (mConfigs.size() <= 2) return;
    if (mConfigs.last().isHuman) return;
    mConfigs.pop_back();
    auto* w = mSlotWidgets.takeLast();
    mSlotsLayout->removeWidget(w);
    delete w;
}

void SetupWidget::onStartClicked() {
    for (int i = 0; i < mConfigs.size(); ++i) {
        if (!mConfigs[i].isHuman &&
            mConfigs[i].strategy == PlayerConfig::StrategyType::GA &&
            mConfigs[i].genomePath.isEmpty())
        {
            QMessageBox::warning(this, "Missing genome",
                QString("Seat %1 is set to GA but no genome file is loaded.").arg(i + 1));
            return;
        }
    }
    emit gameStartRequested(mConfigs, mChipsBox->value());
}

} // namespace poker
