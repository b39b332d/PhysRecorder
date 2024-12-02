#include <cstdlib>
#include "mainwindow.h"
#include <QApplication>
#include <QSplashScreen>
#include <QWindow>
int main(int argc, char *argv[])
{
    putenv("OPENCV_VIDEOIO_MSMF_ENABLE_HW_TRANSFORMS=0");
    QApplication app(argc, argv);

    QFile f(":qdarkstyle/dark/darkstyle.qss");

    if (!f.exists()) {
        qDebug() << "Unable to set stylesheet, file not found\n";
    }
    else {
        f.open(QFile::ReadOnly | QFile::Text);
        QTextStream ts(&f);
        app.setStyleSheet(ts.readAll());
    }

    QPixmap pixmap(PROJECT_ROOT_PATH"resources/ECG.jpg");
    QSplashScreen splash(pixmap);
    splash.show();
    app.setWindowIcon(QIcon(QPixmap(PROJECT_ROOT_PATH"resources/icon.png")));

    MainWindow w = MainWindow(splash);
    w.show();

    splash.finish(&w);
    exit(app.exec());
}
