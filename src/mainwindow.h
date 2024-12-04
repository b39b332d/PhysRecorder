#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "capture.h"
#include <QSpinBox>
#include <QDir>
#include <QSplashScreen>
#include <vector>
#include <qcombobox.h>
#include <QFileSystemWatcher>
#include <CameraCapture.h>
#include <MultiSelectComboBox.h>
#include <QCheckBox>
#include <QPushButton>
#include <unordered_set>
#include <SerialReader.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

    bool painted = true;
    QImage m_img;

    void establish_connection();

public:
    MainWindow(QSplashScreen&,QWidget *parent = nullptr);
    ~MainWindow();
private:
    Ui::MainWindow* ui;
    Capture* capture;
    Converter* converter;
    double ts_idx;
    bool isVideoSet;
    bool capStopped;
    bool setSource;
    bool setTsSource;
    bool waitAnalyzing = false;
    bool isCamInfoUpdated = false;
    bool is_sensor_color;
    bool is_profile_depth;
    bool use_camera;
    int stereo_profile_cam_number;
    int totalSourceFileCanBeIndexed;

    int current_camera_idx = 0;
    SignalProcess* signalProcess;
    QLatin1Char zeropad;
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
    double cam_ofs = 0;
    

    QLineEdit* textedit_serialname;
    QLineEdit* textedit_streamname;

    QTimer cam_option_changed_timer;
    uint32_t cam_option_changed = 0;
    void set_sensor_property();

    std::vector<std::string>stream_names;

    bool is_opened = false;

    QTimer* refresh_plot_timer;
    void refreshCameras();
    void saveSignals();
    //void setCamInfo(CameraInfo*);
    //Q_SIGNAL void setColorReady(uint8_t, uint8_t, uint8_t);
    QFileSystemWatcher watcher;
    QString sharedFilePath;
    bool is_signal_processed = true;
    std::string start_record(std::string save_prefix);
    void stop_record();
    QMetaObject::Connection comp_conn;
private slots:
    void onFileChanged(const QString& path);
    bool emitFileSignal(bool,QString p="");
    void setfft(const QImage& image);
    void refresh_plot();

    void on_actionStartTrigger_triggered();
    void on_actionrefreshSerial_triggered();
    void on_actionrefreshCamera_triggered();
    void on_actionstopSerial_triggered();
    //void plotInterpPPG(double,double);
    //void plotInterpRGB(float r, float g, float b, float pos, const QVector<float>& pos_end, double ts);
    void comb_comp_changed(int index);
    //void on_startButton_clicked();
//void on_stopButton_clicked();
    //void capFinished();
    //void detect_sensors(int);
    //void set_stream(int);
    //void detect_profiles(int);
    //void set_profile(int);

    void onCameraSelected(int);
    void onProfileTypeSelected(int);
    void onProfileSelected(int);
    void onRecordToggled();

    private:
        void run_with_call_back(const std::function<void()>& run_in_thread, const std::function<void()>& call_back);
        void lock_camera_info_play(bool lock=true);
        void loadCameraOptions(capture::CameraDevice* device);
        inline void loadCameraOption(capture::CameraDevice* device, capture::CameraDevice::DEVICE_OPTION opt);


    private:
        QCheckBox* camopt_checkBox[(int)capture::CameraDevice::DEVICE_OPTION_CNT];
        QPushButton* camopt_pushButton[(int)capture::CameraDevice::DEVICE_OPTION_CNT];
        QSlider* camopt_slider[(int)capture::CameraDevice::DEVICE_OPTION_CNT];


public slots:
        void on_capture_device_disabled(capture::CameraDevice*);
        void on_device_selected(capture::CameraDevice*);
        void onSerialStopped(SerialReader* reader);
        void onSerialSelected(int, bool);
        void onSerialHighted(int idx, bool is_highlight);

        void onStreamSelected(int, bool is_selected);
        void onStreamHighted(int, bool);
        void onSerialGraphSelectionChanged();

        void setSqi(int, float, float);
};


#endif // MAINWINDOW_H
