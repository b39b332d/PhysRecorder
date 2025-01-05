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
#include <CameraCapture.h>
#include <QTimer>
#include <QThread>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE
class Capture;
class MultiSelectComboBox;
class Converter;
class SignalProcess;
class SerialReader;
class Worker : public QObject {
    Q_OBJECT

public:
    Worker() : m_running(false) {}

    // Function to run a task in the worker thread
    void run_in_thread(const std::function<void()>& task);
    Q_SIGNAL void process_finished();

private:
    std::atomic<bool> m_running;
};
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QSplashScreen&,QWidget *parent = nullptr);
    ~MainWindow();
private:
    Ui::MainWindow* ui;
    Capture* capture;
    Converter* converter;

    SignalProcess* signalProcess;
    QComboBox* comboBox_profile;
    QComboBox* comboBox_profile_type;
    std::unordered_set<SerialReader*> rec_SerialReaders;
    QComboBox* comboBox_cameras;
    MultiSelectComboBox* comboBox_stream;
    MultiSelectComboBox* comboBox_serial;
    QSpinBox* spinRecordTime;
    QLineEdit* filenameLineEdit;
    QString filenameLineEdit_name;
    QTimer* record_timer;
    int spin_record_last_time;
    double show_window_length = 6.0;
    

    QLineEdit* textedit_serialname;
    QLineEdit* textedit_streamname;

    QTimer cam_option_changed_timer;
    uint32_t cam_option_changed = 0;
    void set_sensor_property();


    QTimer* refresh_plot_timer;

    QFileSystemWatcher watcher;
    QString sharedFilePath;

    std::string start_record(std::string save_prefix);
    void stop_record();
private slots:
    void onFileChanged(const QString& path);
    bool emitFileSignal(bool,QString p="");
    void setfft(const QImage& image);
    void refresh_plot();

    void on_actionStartTrigger_triggered();
    void on_actionrefreshSerial_triggered();
    void on_actionrefreshCamera_triggered();
    void on_actionstopSerial_triggered();
    void comb_comp_changed();

    void onCameraSelected(int);
    void onProfileTypeSelected(int);
    void onProfileSelected(int);
    void onRecordToggled();

    private:
        QThread process_thread;
        Worker process_thread_worker;
        void run_with_call_back(const std::function<void()>& run_in_thread, const std::function<void()>& call_back);

        void lock_camera_info_play(bool lock=true);
        void loadCameraOptions(capture::CameraDevice* device);

    private:
        QCheckBox* camopt_checkBox[(int)capture::CameraDevice::DEVICE_OPTION_CNT];
        QPushButton* camopt_pushButton[(int)capture::CameraDevice::DEVICE_OPTION_CNT];
        QSlider* camopt_slider[(int)capture::CameraDevice::DEVICE_OPTION_CNT];

protected:
    void closeEvent(QCloseEvent* event) override;

public slots:
        void onCaptureDeviceDisabled(capture::CameraDevice*);
        void onDeviceSelected(capture::CameraDevice*);
        void onSerialStopped(SerialReader* reader);
        void onSerialSelected(int, bool);
        void onSerialHighted(int idx, bool is_highlight);

        void onStreamSelected(int, bool is_selected);
        void onStreamHighted(int, bool);
        void onSerialGraphSelectionChanged();

        void setSqi(int, float, float);
};

#endif // MAINWINDOW_H
