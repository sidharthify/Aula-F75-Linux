#include "ui/mainwindow.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("aula-f75"));
    QApplication::setApplicationDisplayName(QStringLiteral("AULA F75"));
    QApplication::setOrganizationName(QStringLiteral("Aula"));
    QApplication::setDesktopFileName(QStringLiteral("org.aula.F75"));
    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("input-keyboard"),
                                                 QIcon(QStringLiteral(":/org.aula.F75.svg"))));

    f75::MainWindow window;
    window.show();

    return app.exec();
}
