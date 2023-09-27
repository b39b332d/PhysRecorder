#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "capture.h"
#include <QSpinBox>
#include <QDir>
#include <QSplashScreen>
#include <vector>
#include<qcombobox.h>
#include <librealsense2/rs.hpp>
#include "ppg.h"
#include "respi.h"
#include "cnpy.h"
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
    int stereo_profile_cam_number;
    int totalSourceFileCanBeIndexed;
    CameraInfo* cameras[10];
    int current_camera_idx = 0;
    SignalProcess* signalProcess;
    QLatin1Char zeropad;
    QComboBox* comboBox_profile;
    QComboBox* comboBox_stream;
    QComboBox* comboBox_cameras;
    QComboBox* comboBox_sensor;
    QSpinBox* spinRecordTime;
    QLineEdit* filenameLineEdit;
    PPGReader* ppg;
    RESPIReader* respi;
    QTimer* record_timer;
    bool isRecording = false;
    int spin_record_last_time;
    double show_window_length = 6.0;
    double cam_ofs = 0;
    
    QTimer cam_option_changed_timer;
    uint16_t cam_option_changed = 0;
    void set_sensor_property();

    std::vector<std::string>stream_names;
    std::vector<uint16_t> ppg_sig_rec;
    std::vector<uchar> respi_sig_rec;
    std::vector<double> ppg_ts_rec;
    std::vector<double> respi_ts_rec;
    bool is_opened = false;

    rs2::device_list devices;
    rs2::device device;
    std::vector<int> profiles_ofs;
    std::vector<rs2::stream_profile> profiles;
    std::vector<rs2::sensor> sensors;
    rs2::stream_profile profile;
    rs2::sensor sensor;
    QTimer* refresh_plot_timer;
    void refreshRealsenseDevices();
    void freshSerialDevices();
    void saveSignals();
    //void setCamInfo(CameraInfo*);
    //Q_SIGNAL void setColorReady(uint8_t, uint8_t, uint8_t);
private slots:
    void set_time_offset(double);
    void setfft(QImage);
    void refresh_plot();
    void plotPPG(uint16_t,double);
    void plotRESPI(uchar,double);
    void on_actionStartTrigger_triggered();
    void on_actionRecord_triggered();
    void detect_profiles(int);
    void detect_sensors(int);
    void set_profile(int);
    void set_stream(int);
    void on_actionrefreshSerial_triggered();
    void on_actionrefreshRealsense_triggered();
    void on_actionstopSerial_triggered();
    void plotInterpPPG(double,double);
    void plotInterpRGB(float, float, float, double, float*);
    //void on_startButton_clicked();
//void on_stopButton_clicked();
    //void capFinished();
};
#endif // MAINWINDOW_H
