#include "ui/PokerQtTypes.h"
#include "ui/MainWindow.h"
#include "ui/SetupWidget.h"
#include "ui/Theme.h"
#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Poker");
    app.setStyle("Fusion");
    poker::applyTheme(app);

    qRegisterMetaType<poker::Action>("poker::Action");
    qRegisterMetaType<poker::GameStateView>("poker::GameStateView");
    qRegisterMetaType<poker::PlayerConfig>("poker::PlayerConfig");
    qRegisterMetaType<QVector<poker::PlayerConfig>>("QVector<poker::PlayerConfig>");

    poker::MainWindow window;
    window.show();
    return app.exec();
}
