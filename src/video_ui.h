#pragma once


#include <CameraCapture.h>
#include <color_extractor.hpp>
#include <QCheckBox>
#include <QObject>
#include <QPushButton>
#include <QSlider>
#include <QLineEdit>
#include <worker_handler.h>
#include <QTimer>
#include <qcombobox.h>

QT_BEGIN_NAMESPACE
namespace Ui { class Video; class MainWindow;}
QT_END_NAMESPACE


class Capture;
class Converter;
class MultiSelectComboBox;
class SignalProcess;
class QToolBar;
class VideoUI :public QWidget {

	Q_OBJECT
public:

	VideoUI(QWidget* parent = nullptr);
	~VideoUI();
	void init(Ui::MainWindow* main_ui, SignalProcess* signalProcess);
	void start_record(const std::string& save_prefix);
	void stop_record();
private:

	QCheckBox* camopt_checkBox[(int)capture::DEVICE_OPTION_CNT];
	QPushButton* camopt_pushButton[(int)capture::DEVICE_OPTION_CNT];
	QSlider* camopt_slider[(int)capture::DEVICE_OPTION_CNT];

	QCheckBox* streamopt_checkBox[capture::STREAM_OPTION_CNT- capture::STREAM_CONTRAST];
	QPushButton* streamopt_pushButton[capture::STREAM_OPTION_CNT - capture::STREAM_CONTRAST];
	QSlider* streamopt_slider[capture::STREAM_OPTION_CNT - capture::STREAM_CONTRAST];

	Capture* capture;
	ColorExtractor* face_tracking;
	Converter* converter;

	QComboBox* comboBox_profile;
	QComboBox* comboBox_profile_type;
	MultiSelectComboBox* comboBox_cameras;
	MultiSelectComboBox* comboBox_stream;
	QLineEdit* textedit_streamname;
	QToolBar* toolBarCamera;


	QAction* actionStartCamera;
	QAction* actionrefreshCamera;
	QPushButton* actionTracking;

	QTimer cam_option_changed_timer;
	uint32_t cam_option_changed = 0;
	uint32_t stream_option_changed = 0;

	Worker worker;

	void set_sensor_property();



	void LoadStreamProfiles(capture::CameraStream* stream);
	void switchStreamStatus(capture::CameraStream* stream, bool open);
	void switchCameraStatus(capture::CameraDevice* camera, bool open);

	void lock_camera_info_play(bool lock = true);
	void loadCameraOptions(capture::CameraStream* stream);
	void connect_option_set(int opt, QSlider** sliders, QCheckBox** checkboxs, QPushButton** pushButtons
		, capture::Options* option, uint32_t&);

	Ui::Video* ui;
private slots:
	void onActionStartCamera();
	void onActionRefreshCamera();
	void encoder_changed();
	void on_convert_set_roi(cv::Rect rect);
	void onCameraSelected(int, bool);
	void onCameraHighLighted(int, bool);
	void onProfileTypeSelected(int);
	void onProfileSelected(int);
	void onCaptureDeviceDisabled(capture::CameraDevice*);
	void onDeviceSelected(capture::CameraStream*);
	void onStreamSelected(int, bool is_selected);
	void onStreamHighLighted(int, bool);

};	