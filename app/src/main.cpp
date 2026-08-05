// AutoTasks — Qt application entry point.
//
// This is the orchestrator + UI process. It commands the engine and reads the
// store; it never synthesizes input itself.

#include "app/MainWindow.h"

#include <QApplication>

int main(int argc, char* argv[]) {
    const QApplication app(argc, argv);

    QCoreApplication::setApplicationName(QStringLiteral("AutoTasks"));
    QCoreApplication::setOrganizationName(QStringLiteral("AutoTasks"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    autotasks::MainWindow window;
    window.show();

    return QApplication::exec();
}
