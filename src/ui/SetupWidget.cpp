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
#include <QFrame>

namespace poker {

SetupWidget::SetupWidget(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setSpacing(0);
    root->setContentsMargins(0, 0, 0, 0);

    // ── Centred content column ─────────────────────────────────────────────
    auto* outer = new QHBoxLayout;
    outer->setContentsMargins(0, 0, 0, 0);
    auto* col = new QVBoxLayout;
    col->setSpacing(20);
    col->setContentsMargins(0, 40, 0, 40);
    outer->addStretch(1);
    outer->addLayout(col, 0);
    outer->addStretch(1);
    root->addStretch(1);
    root->addLayout(outer);
    root->addStretch(1);

    // ── Header ─────────────────────────────────────────────────────────────
    auto* title = new QLabel("POKER", this);
    title->setObjectName("titleLabel");
    title->setAlignment(Qt::AlignCenter);

    auto* subtitle = new QLabel("CONFIGURE TABLE", this);
    subtitle->setObjectName("subtitleLabel");
    subtitle->setAlignment(Qt::AlignCenter);

    col->addWidget(title);
    col->addWidget(subtitle);

    // Divider
    auto* divider = new QFrame(this);
    divider->setObjectName("divider");
    divider->setFixedWidth(360);
    col->addWidget(divider, 0, Qt::AlignHCenter);

    // ── Players group ──────────────────────────────────────────────────────
    auto* slotsGroup = new QGroupBox("PLAYERS", this);
    slotsGroup->setFixedWidth(440);
    mSlotsLayout = new QVBoxLayout(slotsGroup);
    mSlotsLayout->setSpacing(8);
    mSlotsLayout->setContentsMargins(12, 8, 12, 12);
    col->addWidget(slotsGroup, 0, Qt::AlignHCenter);

    // Add/remove buttons
    auto* slotBtnRow = new QHBoxLayout;
    slotBtnRow->setSpacing(10);
    mAddBtn = new QPushButton("+ Add player", this);
    mAddBtn->setObjectName("addBtn");
    mRemoveBtn = new QPushButton("− Remove last", this);
    mRemoveBtn->setObjectName("removeBtn");
    slotBtnRow->addWidget(mAddBtn);
    slotBtnRow->addWidget(mRemoveBtn);
    slotBtnRow->addStretch();

    auto* slotBtnWrapper = new QWidget(this);
    slotBtnWrapper->setFixedWidth(440);
    slotBtnWrapper->setLayout(slotBtnRow);
    col->addWidget(slotBtnWrapper, 0, Qt::AlignHCenter);

    // ── Starting chips ─────────────────────────────────────────────────────
    auto* chipsRow = new QHBoxLayout;
    chipsRow->setSpacing(12);
    auto* chipsLbl = new QLabel("STARTING CHIPS", this);
    chipsLbl->setObjectName("sectionLabel");
    mChipsBox = new QSpinBox(this);
    mChipsBox->setRange(100, 1'000'000);
    mChipsBox->setValue(10000);
    mChipsBox->setSingleStep(1000);
    chipsRow->addWidget(chipsLbl);
    chipsRow->addStretch();
    chipsRow->addWidget(mChipsBox);

    auto* chipsWrapper = new QWidget(this);
    chipsWrapper->setFixedWidth(440);
    chipsWrapper->setLayout(chipsRow);
    col->addWidget(chipsWrapper, 0, Qt::AlignHCenter);

    // Divider
    auto* divider2 = new QFrame(this);
    divider2->setObjectName("divider");
    divider2->setFixedWidth(360);
    col->addWidget(divider2, 0, Qt::AlignHCenter);

    // ── Start button ───────────────────────────────────────────────────────
    mStartBtn = new QPushButton("START GAME", this);
    mStartBtn->setObjectName("startBtn");
    mStartBtn->setFixedWidth(280);
    col->addWidget(mStartBtn, 0, Qt::AlignHCenter);

    // ── Connections ────────────────────────────────────────────────────────
    connect(mAddBtn,    &QPushButton::clicked, this, &SetupWidget::addPlayerSlot);
    connect(mRemoveBtn, &QPushButton::clicked, this, &SetupWidget::removeLastSlot);
    connect(mStartBtn,  &QPushButton::clicked, this, &SetupWidget::onStartClicked);

    // Default: human + 1 Random opponent
    PlayerConfig human; human.isHuman = true;
    mConfigs.push_back(human);
    buildSlotRow(0);

    PlayerConfig ai; ai.strategy = PlayerConfig::StrategyType::Random;
    mConfigs.push_back(ai);
    buildSlotRow(1);
}

void SetupWidget::buildSlotRow(int index) {
    auto* row    = new QWidget(this);
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(10);

    bool isHuman = mConfigs[index].isHuman;
    auto* label  = new QLabel(isHuman ? "You (Human)" : QString("Seat %1").arg(index + 1), row);
    label->setObjectName("nameLabel");
    label->setMinimumWidth(110);
    layout->addWidget(label);

    if (!isHuman) {
        auto* combo = new QComboBox(row);
        combo->addItem("Random",     static_cast<int>(PlayerConfig::StrategyType::Random));
        combo->addItem("MonteCarlo", static_cast<int>(PlayerConfig::StrategyType::MonteCarlo));
        combo->addItem("GA",         static_cast<int>(PlayerConfig::StrategyType::GA));
        combo->setCurrentIndex(static_cast<int>(mConfigs[index].strategy));
        layout->addWidget(combo);

        auto* fileBtn   = new QPushButton("Load genome...", row);
        auto* fileLabel = new QLabel("(none)", row);
        fileLabel->setObjectName("fileLabel");
        layout->addWidget(fileBtn);
        layout->addWidget(fileLabel);

        connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [fileBtn, fileLabel, combo](int) {
                bool isGA = combo->currentData().toInt() ==
                            static_cast<int>(PlayerConfig::StrategyType::GA);
                fileBtn->setVisible(isGA);
                fileLabel->setVisible(isGA);
            });

        connect(fileBtn, &QPushButton::clicked, this, [this, index, fileLabel]() {
            QString path = QFileDialog::getOpenFileName(
                this, "Load Genome", "", "Genome files (*.genome)");
            if (!path.isEmpty()) {
                mConfigs[index].genomePath = path;
                fileLabel->setText(QFileInfo(path).fileName());
                fileLabel->setStyleSheet("color: #70d898; font-size: 11px;");
            }
        });

        connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this, index, combo](int) {
                mConfigs[index].strategy =
                    static_cast<PlayerConfig::StrategyType>(combo->currentData().toInt());
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
