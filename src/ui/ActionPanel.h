#pragma once
#include "ui/PokerQtTypes.h"
#include <QWidget>

class QPushButton;
class QSpinBox;

namespace poker {

class ActionPanel : public QWidget {
    Q_OBJECT
public:
    explicit ActionPanel(QWidget* parent = nullptr);
    void activate(int minToCall, int pot, int playerStack);
    void deactivate();

signals:
    void actionChosen(poker::Action action);

private slots:
    void onFold();
    void onCall();
    void onRaise();

private:
    QPushButton* mFoldBtn;
    QPushButton* mCallBtn;
    QPushButton* mRaiseBtn;
    QSpinBox*    mBetBox;
    int          mMinToCall = 0;
};

} // namespace poker
