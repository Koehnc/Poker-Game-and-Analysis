#pragma once
#include <QWidget>
#include <QVector>
#include <QString>

class QPushButton;
class QSpinBox;
class QVBoxLayout;

namespace poker {

struct PlayerConfig {
    enum class StrategyType { Random, MonteCarlo, GA };
    StrategyType strategy = StrategyType::MonteCarlo;
    QString      genomePath;
    bool         isHuman = false;
};

class SetupWidget : public QWidget {
    Q_OBJECT
public:
    explicit SetupWidget(QWidget* parent = nullptr);

signals:
    void gameStartRequested(QVector<poker::PlayerConfig> players, int startingChips);

private slots:
    void addPlayerSlot();
    void removeLastSlot();
    void onStartClicked();

private:
    void buildSlotRow(int index);

    QVBoxLayout*          mSlotsLayout;
    QVector<QWidget*>     mSlotWidgets;
    QVector<PlayerConfig> mConfigs;
    QSpinBox*             mChipsBox;
    QPushButton*          mAddBtn;
    QPushButton*          mRemoveBtn;
    QPushButton*          mStartBtn;
};

} // namespace poker

Q_DECLARE_METATYPE(poker::PlayerConfig)
Q_DECLARE_METATYPE(QVector<poker::PlayerConfig>)
