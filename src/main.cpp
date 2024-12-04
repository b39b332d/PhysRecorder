#include <cstdlib>
#include "mainwindow.h"
#include <QApplication>
#include <QSplashScreen>
#include <QWindow>
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QPixmap pixmap(PROJECT_ROOT_PATH"resources/ECG.jpg");
    QSplashScreen splash(pixmap);
    splash.show();


    QFile f(":qdarkstyle/dark/darkstyle.qss");

    if (!f.exists()) {
        qDebug() << "Unable to set stylesheet, file not found\n";
    }
    else {
        f.open(QFile::ReadOnly | QFile::Text);
        QTextStream ts(&f);
        app.setStyleSheet(ts.readAll());
    }

    app.setWindowIcon(QIcon(QPixmap(PROJECT_ROOT_PATH"resources/icon.png")));

    MainWindow w = MainWindow(splash);
    w.show();
    splash.finish(&w);
    exit(app.exec());
}
