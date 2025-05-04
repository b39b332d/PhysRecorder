#include <cstdlib>
#include <QApplication>
#include <QSplashScreen>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#ifdef _WIN32
  #define DLL_IMPORT __declspec(dllimport)
  DLL_IMPORT void start_mainwin(QSplashScreen* screen,QString* program_path);
#else
  #include <dlfcn.h>
  #define DLL_IMPORT 
#endif

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QString program_path = QCoreApplication::applicationDirPath()+"/";
    QPixmap pixmap(program_path+"data/resources/ECG.bmp");
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

    app.setWindowIcon(QIcon(QPixmap(program_path+"data/resources/icon.ico")));

    #ifdef _WIN32
    start_mainwin(&splash,&program_path);
    #else
        void *handle;
        void (*start_mainwin)(QSplashScreen* screen,QString* program_path);

        handle = dlopen ("libPhysRecorder_main.so", RTLD_LAZY);
        if (!handle) {
            exit(1);
        }
        start_mainwin = (void (*)(QSplashScreen* screen,QString* program_path))dlsym(handle, "start_mainwin");
        if (dlerror() != NULL)  {
            exit(1);
        }

        (*start_mainwin)(&splash,&program_path);
    #endif
    exit(app.exec());
}
