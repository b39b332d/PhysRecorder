#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSpinBox>
#include <QDir>
#include <QSplashScreen>
#include <QFileSystemWatcher>
#include <QCheckBox>
#include <qcombobox.h>
#include <QPushButton>
#include <unordered_set>
#include <vector>
#include <QTimer>
#include <QThread>
#include <worker_handler.h>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE
class MultiSelectComboBox;
class SignalProcess;
class SerialReader;
class VideoUI;
class MaterialColorManager;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QSplashScreen*,QWidget *parent = nullptr);
    ~MainWindow();
    Ui::MainWindow* ui;

    MaterialColorManager* color_manager;

    std::unordered_set<SerialReader*> rec_SerialReaders;
    MultiSelectComboBox* comboBox_serial;
    QSpinBox* spinRecordTime;
    QLineEdit* filenameLineEdit;
    QString filenameLineEdit_name;
    QTimer* record_timer;
    int spin_record_last_time;
    double show_window_length = 6.0;
    SignalProcess* signalProcess;
    VideoUI* videoui;
    
    QLineEdit* textedit_serialname;

    QTimer* refresh_plot_timer;

    QFileSystemWatcher watcher;
    QString sharedFilePath;
    int busy_count = 0;

    std::string start_record(std::string save_prefix);
    void stop_record();
    Worker worker;
private slots:
    void onFileChanged(const QString& path);
    bool emitFileSignal(bool,QString p="");
    void setfft(const QImage& image);
    void refresh_plot();

    void on_actionrefreshSerial_triggered();
    void on_actionstopSerial_triggered();

    void onRecordToggled();


protected:
    void closeEvent(QCloseEvent* event) override;

public slots:
    void onSerialStopped(SerialReader* reader);
    void onSerialSelected(int, bool);
    void onSerialHighted(int idx, bool is_highlight);

    void onSerialGraphSelectionChanged();

    void setSqi(int, float, float);
};

#endif // MAINWINDOW_H
