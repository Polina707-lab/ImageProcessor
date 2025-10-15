#include <QApplication>
#include <QFile>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QFile styleFile(":/style.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        app.setStyleSheet(styleFile.readAll());
    }


    MainWindow w;
    w.show();
    return app.exec();
}
