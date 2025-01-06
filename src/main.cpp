#include <cstdlib>
#include <QApplication>
#include <QSplashScreen>
#include <QMessageBox>
#include <QFile>
#include <QDir>
__declspec(dllimport) void start_mainwin(QSplashScreen& screen);
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QPixmap pixmap(PROJECT_ROOT_PATH"resources/ECG.bmp");
    QSplashScreen splash(pixmap);
    splash.show();
    if (!QDir("./rec").exists()) {
        bool s = QDir().mkdir("./rec");
        if (!s) {
            QMessageBox::critical(nullptr, "Error", "Can not create rec folder!");
            app.exit();
            return 0;
        }
    }
    splash.showMessage("Loading DLLs");


    QFile f(":qdarkstyle/dark/darkstyle.qss");

    if (!f.exists()) {
        qDebug() << "Unable to set stylesheet, file not found\n";
    }
    else {
        f.open(QFile::ReadOnly | QFile::Text);
        QTextStream ts(&f);
        app.setStyleSheet(ts.readAll());
    }

    app.setWindowIcon(QIcon(QPixmap(PROJECT_ROOT_PATH"resources/icon.ico")));

    start_mainwin(splash);
    exit(app.exec());
}
