#include "video_ui.h"
#include "MultiSelectComboBox.h"
#include <converter.h>
#include <capture.h>
#include <RPPGExtractor.h>
#include "./ui_mainwindow.h"
#include "./ui_video.h"
#include "signalprocess.h"
#include <SilentLineEdit.h>
VideoUI::VideoUI(QWidget* parent)
    :QWidget(parent), worker(this), ui(new Ui::Video) 
{
    ui->setupUi(this);
}

void VideoUI::onConvertSetROI(cv::Rect rect) {
    if (ui->pushButton_crop_roi->isChecked()) {
        if (rect.width > 100 && rect.height > 100) {
            unsigned x = ui->lineEdit_crop_roi->property("roi_x").toUInt();
            unsigned y = ui->lineEdit_crop_roi->property("roi_y").toUInt();
            ui->lineEdit_crop_roi->setText(QString::asprintf("(%d,%d,%d,%d)", rect.x+x, rect.y+y, rect.width, rect.height));
            emit ui->lineEdit_crop_roi->editingFinished();
        }
        ui->tab_postprocess->setEnabled(true);
        ui->pushButton_crop_roi->setChecked(false);
    }
    else {
        if (rect.area() == 0) {
            actionTracking->setChecked(false);
            emit actionTracking->clicked(false);
        }
        else {
            actionTracking->setText("ROI");
            actionTracking->setChecked(true);
            face_tracking->set_roi(rect);
        }
    }
}
void VideoUI::init(Ui::MainWindow* main_ui, SignalProcess* signalProcess)
{
    this->actionStartCamera = main_ui->actionStartCamera;
    this->actionrefreshCamera = main_ui->actionrefreshCamera;
    this->actionTracking = main_ui->actionTracking;
    this->toolBarCamera = main_ui->toolBarCamera;
    actionStartCamera->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    actionrefreshCamera->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));

    comboBox_cameras = new MultiSelectComboBox(toolBarCamera);
    comboBox_stream = new MultiSelectComboBox(toolBarCamera);
    comboBox_profile_type = new QComboBox(toolBarCamera);
    comboBox_profile = new QComboBox(toolBarCamera);
    comboBox_cameras->setFixedWidth(225);
    comboBox_stream->setFixedWidth(110);
    comboBox_profile_type->setFixedWidth(60);
    comboBox_profile->setFixedWidth(130);
    toolBarCamera->addWidget(comboBox_cameras);
    toolBarCamera->addWidget(comboBox_stream);
    toolBarCamera->addWidget(comboBox_profile_type);
    toolBarCamera->addWidget(comboBox_profile);
    connect(comboBox_cameras, &MultiSelectComboBox::highLightSelect, this, &VideoUI::onCameraHighLighted);
    connect(comboBox_cameras, &MultiSelectComboBox::selectionChanged, this, &VideoUI::onCameraSelected);
    connect(comboBox_stream, &MultiSelectComboBox::highLightSelect, this, &VideoUI::onStreamHighLighted);
    connect(comboBox_stream, &MultiSelectComboBox::selectionChanged, this, &VideoUI::onStreamSelected);
    connect(comboBox_profile_type, &QComboBox::activated, this, &VideoUI::onProfileTypeSelected);
    connect(comboBox_profile, &QComboBox::activated, this, &VideoUI::onProfileSelected);

    connect(actionStartCamera, &QAction::triggered, this, &VideoUI::onActionStartCamera);
    connect(actionrefreshCamera, &QAction::triggered, this, &VideoUI::onActionRefreshCamera);


    textedit_streamname = new QLineEdit(toolBarCamera);
    textedit_streamname->setMaximumWidth(200);
    textedit_streamname->setDisabled(true);
    textedit_streamname->setPlaceholderText("Device Filename");
    toolBarCamera->addWidget(textedit_streamname);


    onActionRefreshCamera();


    converter = new Converter(ui->q_video);
    QThread* converterThread = new QThread();

    face_tracking = new ColorExtractor(Inference::get_inference(INF_OPENVINO));
    capture = new Capture(face_tracking);
    converter->moveToThread(converterThread);
    connect(capture, &Capture::device_disabled, this, &VideoUI::onCaptureDeviceDisabled);
    connect(capture, &Capture::updateFrame, converter, &Converter::frame_ready);
    connect(converter, &Converter::frameReady, ui->q_video, &ImageViewer::setImage);
    connect(converter, &Converter::stream_selected, this, &VideoUI::onDeviceSelected);

    converterThread->start();

    connect(converter, &Converter::set_roi, this, &VideoUI::onConvertSetROI);

    connect(face_tracking, &ColorExtractor::on_signal_ready, signalProcess, &SignalProcess::processSignal);
    connect(face_tracking, &ColorExtractor::on_face_lost, signalProcess, &SignalProcess::reset_rppg);

    connect(actionTracking, &QPushButton::clicked, this, [this](bool checked) {
        if (checked) {
            actionTracking->setText("Tracking");
            face_tracking->set_roi({ 0,0,0,0 });
        }
        else {
            actionTracking->setText("Untracked");
            face_tracking->disable_tracking();
        }
        });

    cam_option_changed_timer.setSingleShot(false);
#define match_array_qobj_camopts(obj,dev,name) camopt_##obj[capture::DEVICE_##dev] = ui->obj##_##name

#define  match_array_camopts(obj) \
    match_array_qobj_camopts(obj, EXPOSURE, exposure);\
    match_array_qobj_camopts(obj, GAIN, gain);\
    match_array_qobj_camopts(obj, WHITE_BALANCE, whiteBalance);\
    match_array_qobj_camopts(obj, GAMMA, gamma);\
    match_array_qobj_camopts(obj, LIGHT, light);\
    match_array_qobj_camopts(obj, ZOOM, zoom);\
    match_array_qobj_camopts(obj, PAN, pan);\
    match_array_qobj_camopts(obj, TILT, tilt);\
    match_array_qobj_camopts(obj, ROLL, roll);\
    match_array_qobj_camopts(obj, IRIS, iris);\
    match_array_qobj_camopts(obj, FOCUS, focus);\
    match_array_qobj_camopts(obj, CONTRAST, contrast);\
    match_array_qobj_camopts(obj, HUE, hue);\
    match_array_qobj_camopts(obj, SATURATION, saturation);\
    match_array_qobj_camopts(obj, SHARPNESS, sharpness);\
    match_array_qobj_camopts(obj, BRIGHTNESS, brightness);

    match_array_camopts(checkBox);
    match_array_camopts(pushButton);
    match_array_camopts(slider);


    match_array_qobj_camopts(checkBox, BACKLIGHT, backlight);
    match_array_qobj_camopts(checkBox, COLOR_ENABLED, colorEnabled);
    match_array_qobj_camopts(pushButton, BACKLIGHT, backlight);
    match_array_qobj_camopts(pushButton, COLOR_ENABLED, colorEnabled);


#define match_qobj_streamopts(n,name) \
    streamopt_checkBox[n] = ui->checkBox_p##name; \
    streamopt_slider[n] = ui->slider_p##name; \
    streamopt_pushButton[n] = ui->pushButton_p##name

    match_qobj_streamopts(capture::STREAM_CONTRAST, contrast);
    match_qobj_streamopts(capture::STREAM_BRIGHTNESS, brightness);
    match_qobj_streamopts(capture::STREAM_SHARPNESS, sharpness);
    match_qobj_streamopts(capture::STREAM_GAMMA, gamma);
    match_qobj_streamopts(capture::STREAM_SATURATION, saturation);
    match_qobj_streamopts(capture::STREAM_VALUE, value);


    connect(ui->pushButton_resetVideo, &QPushButton::clicked, this, [this]() {
        for (int opt = 8; opt < capture::DEVICE_OPTION_CNT; opt++) {
            if (camopt_pushButton[opt]->isEnabled()) {
                emit camopt_pushButton[opt]->clicked();
            }
        }
        });
    connect(ui->pushButton_resetCamera, &QPushButton::clicked, this, [this]() {
        for (int opt = 0; opt < 8; opt++) {
            if (camopt_pushButton[opt]->isEnabled()) {
                emit camopt_pushButton[opt]->clicked();
            }
        }
        });
    connect(ui->pushButton_process_reset, &QPushButton::clicked, this, [this]() {
        if (ui->pushButton_process_mode->text() == "Lossy") {
            emit ui->pushButton_process_mode->clicked();
        }
        else {
            emit ui->pushButton_process_rot->clicked();
        }

        ui->checkBox_process_fliplr->setChecked(false);
        ui->checkBox_process_flipud->setChecked(false);
        ui->lineEdit_crop_roi->setText("-1");
        emit ui->lineEdit_crop_roi->editingFinished();

        for (int opt = 0; opt < capture::STREAM_LOSSY_OPTION_CNT; opt++) {
            emit streamopt_pushButton[opt]->clicked();
        }

        });

    connect(&cam_option_changed_timer, &QTimer::timeout, this, [this]() {
        cam_option_changed_timer.stop();
        set_sensor_property();
        });


    connect(ui->scroll_rotation, &QScrollBar::valueChanged, this, [this](int val) {
        capture->rot = val;
        this->ui->button_rotation->setText(QString::number(val) + "\xc2\xb0");
        });
    connect(ui->slider_scale, &QSlider::valueChanged, this, [this](int val) {
        capture->scale = (double)val/100.0;
        ui->pushButton_scale->setText(QString::number(capture->scale));
        });


    connect(ui->comboBox_codec, &QComboBox::currentIndexChanged, this, &VideoUI::encoder_changed);

    connect(ui->button_rotation, &QPushButton::clicked, this, [this]() {
        this->ui->scroll_rotation->setValue(0);
        });

    connect(ui->pushButton_scale, &QPushButton::clicked, this, [this]() {
        ui->slider_scale->setValue(100);
        });

    connect(ui->checkBox_fliplr, &QCheckBox::toggled, this, [this](bool checked) {
        capture->is_fliplr = checked;
        });
    connect(ui->checkBox_flipud, &QCheckBox::toggled, this, [this](bool checked) {
        capture->is_flipud = checked;
        });

    connect(ui->pushButton_pre_reset, &QPushButton::clicked, this, [this]() {
        emit ui->pushButton_scale->clicked();
        emit ui->button_rotation->clicked();
        if(ui->checkBox_fliplr->isChecked())
            emit ui->checkBox_fliplr->toggled(false);
        if (ui->checkBox_flipud->isChecked())
            emit ui->checkBox_flipud->toggled(false);
        if (ui->comboBox_inf_method->currentIndex() != 0)
            ui->comboBox_inf_method->setCurrentIndex(0);

        });

    connect(ui->comboBox_inf_method, &QComboBox::currentIndexChanged, this, [this, signalProcess](int idx) {
        setCursorBusy(true);
        disconnect(face_tracking);
        worker.run_with_call_back([idx,this]() {
            capture->setInference(nullptr);
            delete face_tracking;
            if (idx == 0)
                face_tracking = new ColorExtractor(Inference::get_inference(INF_OPENVINO));
            else
                face_tracking = new ColorExtractor(Inference::get_inference(INF_DLIB));

            actionTracking->setChecked(false);
            emit actionTracking->clicked(false);
            capture->setInference(face_tracking);
            }, [this, signalProcess]() {
                connect(face_tracking, &ColorExtractor::on_signal_ready, signalProcess, &SignalProcess::processSignal);
                connect(face_tracking, &ColorExtractor::on_face_lost, signalProcess, &SignalProcess::reset_rppg);
                setCursorBusy(false);
                }
                );
        });

    ui->VideoBox->hide();
    ui->VideoBox->setDisabled(true);

    worker.wait();

}

VideoUI::~VideoUI() {

}

void VideoUI::start_record(const std::string& save_prefix)
{
    setCursorBusy(true);
    ui->VideoBox->setDisabled(true);

    worker.run_with_call_back(
        [this, save_prefix]() {
            for (int i = 0; i < comboBox_cameras->count(); i++) {
                capture::CameraDevice* device = comboBox_cameras->itemData(i).value<capture::CameraDevice*>();
                if (device->is_running()) {
                    for (auto stream : device->enabled_streams) {
                        auto f_name = save_prefix + stream->stream_friendly_name;
                        std::ranges::replace(f_name, ':', '-');
                        capture->recordStream(stream, f_name);
                    }
                }
            }
            capture->startRecord();
        },
        [this]() {
            setCursorBusy(false);
        }
    );
}

void VideoUI::stop_record()
{
    setCursorBusy(true);

    worker.run_with_call_back(
        [this]() {
            capture->startRecord(false);
        },
        [this]() {
            ui->VideoBox->setDisabled(false);
            setCursorBusy(false);
        }
    );
}

void VideoUI::lock_camera_info_play(bool lock) {
    comboBox_profile_type->setDisabled(lock);
    comboBox_profile->setDisabled(lock);
    comboBox_stream->setDisabled(lock);
    if (lock) {
        ui->VideoBox->setDisabled(false);
        actionStartCamera->setToolTip("Stop");
        actionStartCamera->setIcon(style()->standardIcon(QStyle::SP_MediaStop));

        ui->VideoBox->show();
        textedit_streamname->disconnect(this);
        textedit_streamname->setDisabled(false);
        textedit_streamname->setText(capture->selected_stream->stream_friendly_name.c_str());
        connect(textedit_streamname, &QLineEdit::editingFinished, this, [this]() {
            auto text = textedit_streamname->text().replace(QRegularExpression("[/\\\\:*?\"'<>|]"), "-");
            capture->selected_stream->stream_friendly_name = text.toStdString();
            textedit_streamname->setText(text);
            });
    }
    else {
        ui->VideoBox->setDisabled(true);
        actionStartCamera->setToolTip("Play");
        actionStartCamera->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));

        ui->VideoBox->hide();
        textedit_streamname->disconnect(this);
        textedit_streamname->setDisabled(true);
        textedit_streamname->setText("");
    }
}


void VideoUI::onDeviceSelected(capture::CameraStream* stream) {
    if (stream == nullptr) {
        if (ui->pushButton_crop_roi->isChecked()) {
            ui->tab_postprocess->setEnabled(true);
            ui->pushButton_crop_roi->setChecked(false);
        }
        else {
            comboBox_cameras->setHighLight(-1, false);
            capture->selected_stream = nullptr;
            actionTracking->setChecked(false);
            emit actionTracking->clicked(false);
        }
    }
    else {
        int idx = comboBox_cameras->findData(QVariant::fromValue(stream->device));
        if (idx != -1) {
            comboBox_cameras->setHighLight(idx, true);
            comboBox_stream->selectItem(QVariant::fromValue(stream), true);
        }
    }
}

void VideoUI::onCaptureDeviceDisabled(capture::CameraDevice* device) {
    int idx = comboBox_cameras->findData(QVariant::fromValue(device));
    if (idx != -1 && idx == comboBox_cameras->getHighLight()) {
        lock_camera_info_play(false);
        actionStartCamera->setEnabled(true);
        comboBox_cameras->setIndexSelect(idx, false);
    }
}
void VideoUI::onCameraSelected(int idx, bool is_selected) {
    comboBox_stream->clear();
    comboBox_profile_type->clear();
    comboBox_profile->clear();
    actionStartCamera->setDisabled(true);

    setCursorBusy(true);
    capture::CameraDevice* device = comboBox_cameras->itemData(idx).value<capture::CameraDevice*>();

    emit converter->set_roi({ 0,0,0,0 });
    if (is_selected) {
        comboBox_cameras->setIndexSelect(idx, false);
        worker.run_with_call_back([this, device]() {
            device->init();
            },
            [this, device, idx]() {
                comboBox_cameras->setHighLight(idx, true);
                comboBox_cameras->hide();
                setCursorBusy(false);
            });
    }
    else {
        switchCameraStatus(device, false);
    }
}

void VideoUI::onCameraHighLighted(int idx, bool is_highlight) {

    if (is_highlight) {
        capture::CameraDevice* device = comboBox_cameras->getHighLightData().value<capture::CameraDevice*>();

        for (auto& [stream_name, stream] : device->streams_map) {
            if (device->enabled_streams.contains(stream))
                comboBox_stream->addItem(QString::fromStdString(stream_name), QVariant::fromValue(stream), true);
            else
                comboBox_stream->addItem(QString::fromStdString(stream_name), QVariant::fromValue(stream), false);

        }
        onStreamHighLighted(-1, false);
        actionStartCamera->setDisabled(true);
        setCursorBusy(false);
    }
    else {
        comboBox_stream->clear();
        comboBox_profile_type->clear();
        comboBox_profile->clear();
        actionStartCamera->setDisabled(true);
        lock_camera_info_play(false);
        capture->selected_stream = nullptr;
        return;
    }
}
void VideoUI::switchStreamStatus(capture::CameraStream* stream, bool open) {
    capture::CameraDevice* device = stream->device;
    static bool device_start_successed = false;
    setCursorBusy(true);
    worker.run_with_call_back([&, device, stream, open]() {
        if (device->is_running())
            capture::disable_device(device);
        if (open) {
            auto profile = comboBox_profile->currentData().value<capture::CameraProfile*>();
            device->register_stream(profile);
        }
        else {
            device->unregister_stream(stream);
        }
        device_start_successed = enable_device(device);
        }, [&, device, stream]() {
            if (device_start_successed) {
                comboBox_cameras->selectItem(QVariant::fromValue(device), true);
                capture->selected_stream = stream;
                lock_camera_info_play(true);
            }
            else {
                lock_camera_info_play(false);
            }
            setCursorBusy(false);
            actionStartCamera->setDisabled(false);
            });

}
void VideoUI::switchCameraStatus(capture::CameraDevice* device, bool open) {
    setCursorBusy(true);
    if (open) {
        comboBox_cameras->selectItem(QVariant::fromValue(device), true);
        for (auto i : comboBox_stream->getSelectedItems()) {
            auto stream = comboBox_stream->itemData(i).value<capture::CameraStream*>();
            auto profile = comboBox_profile->currentData().value<capture::CameraProfile*>();
            device->register_stream(profile);
        }
        static bool device_start_successed;
        worker.run_with_call_back([device]() {
            device_start_successed = enable_device(device);
            },
            [this, device]() {
                if (device_start_successed) {
                    capture->selected_stream = comboBox_stream->getHighLightData().value<capture::CameraStream*>();
                    lock_camera_info_play(true);
                }
                actionStartCamera->setDisabled(false);
                setCursorBusy(false);
            });
    }
    else {
        worker.run_with_call_back([device]() {
            disable_device(device);
            },
            [this, device]() {
                comboBox_cameras->selectItem(QVariant::fromValue(device), false);
                setCursorBusy(false);
            });
    }
}
void VideoUI::onStreamSelected(int idx, bool is_selected) {
    capture::CameraStream* stream = comboBox_stream->itemData(idx).value<capture::CameraStream*>();

    if (is_selected) {
        LoadStreamProfiles(stream);
        if (comboBox_stream->getSelectedItems().size() != 0)
            actionStartCamera->setDisabled(false);
        else
            actionStartCamera->setDisabled(true);
    }
    else {
        comboBox_profile_type->clear();
        comboBox_profile->clear();
    }
}
void VideoUI::LoadStreamProfiles(capture::CameraStream* stream) {
    comboBox_profile_type->clear();
    comboBox_profile->clear();
    int item_idx = 0;
    int select_idx = -1;
    auto profile_type_name = stream->get_current_profile()->get_profile_codec();
    for (auto& [profile_name, profiles] : stream->profiles_map) {
        comboBox_profile_type->addItem(QString::fromStdString(profile_name),
            QVariant::fromValue(&profiles));
        if (profile_type_name == profile_name) {
            onProfileTypeSelected(item_idx);
            select_idx = item_idx;
        }
        item_idx++;
    }
    comboBox_profile_type->setCurrentIndex(select_idx);
}
void VideoUI::onStreamHighLighted(int idx, bool is_highlight) {

    if (is_highlight) {
        capture::CameraStream* stream = comboBox_stream->itemData(idx).value<capture::CameraStream*>();
        capture->selected_stream = stream;
        if (stream->device->is_stream_enabled(stream)) {
            LoadStreamProfiles(stream);
            converter->set_roi({ 0,0,0,0 });
            lock_camera_info_play(true);
        }
        actionStartCamera->setDisabled(false);
        loadCameraOptions(stream);
    }
    else {
        comboBox_profile_type->clear();
        comboBox_profile_type->setCurrentIndex(-1);
        comboBox_profile->clear();
        comboBox_profile->setCurrentIndex(-1);
    }
}


void VideoUI::onProfileTypeSelected(int idx) {
    setCursorBusy(true);
    comboBox_profile->clear();


    auto profiles = comboBox_profile_type->itemData(idx).value<std::set<capture::CameraProfile*, capture::CameraStream::Cmp>*>();
    int item_idx = 0;
    int select_idx = -1;
    for (auto& profile : *profiles) {
        if (*(profile->stream->get_current_profile()) == *(profile)) {
            select_idx = item_idx;
        }
        comboBox_profile->addItem(QString::fromStdString(profile->get_profile_str()),
            QVariant::fromValue(profile));
        item_idx++;
    }
    comboBox_profile->setCurrentIndex(select_idx);
    setCursorBusy(false);
}

void VideoUI::onProfileSelected(int idx) {
    //setCursorBusy(true);
    //ui->toolBarCamera->setDisabled(true);

    actionStartCamera->setDisabled(false);
    toolBarCamera->setDisabled(false);
    //setCursorBusy(false);
}
void VideoUI::connect_option_set(int opt,QSlider** sliders,QCheckBox** checkboxs,QPushButton** pushButtons
    , capture::Options* option, uint32_t&cam_option_changed) {

    const capture::option_range& opt_range = option->get_option_range(opt);
    const capture::option_status& current_opt = option->get_option(opt);
    const capture::option_status& reset_prop = option->get_reset_option(opt);
    float factor = opt_range.scaled_factor;
    sliders[opt]->disconnect(this);
    checkboxs[opt]->disconnect(this);
    pushButtons[opt]->disconnect(this);
    if (option->is_supported(opt)) {
        sliders[opt]->setDisabled(false);
        checkboxs[opt]->setDisabled(false);
        pushButtons[opt]->setDisabled(false);
        sliders[opt]->setMinimum(opt_range.min);
        sliders[opt]->setMaximum(opt_range.max);
        sliders[opt]->setSingleStep(opt_range.step);
        int step = opt_range.step;
        int min = opt_range.min;
        if (current_opt.status_type == capture::OPTION_AUTO) {
            checkboxs[opt]->setChecked(false);
            pushButtons[opt]->setText("AUTO");
            sliders[opt]->setEnabled(false);
            sliders[opt]->setValue(opt_range.def.value);
        }
        else {
            checkboxs[opt]->setChecked(true);
            sliders[opt]->setEnabled(true);
            sliders[opt]->setValue(current_opt.value);
            pushButtons[opt]->setText(QString::number(current_opt.value / factor, 'g', 5));
        }
        connect(sliders[opt], &QSlider::valueChanged, this, [sliders,pushButtons, opt, this, &cam_option_changed, factor, step, min](int val) {
            cam_option_changed |= (1 << opt);
            auto ofs = (val - min) % step;
            auto grond = ((val - min) / step) * step + min;
            if (ofs!=0) {
                if ((float)ofs / step > 0.5)
                    val = grond + step;
                else
                    val = grond;
            }
            sliders[opt]->setValue(val);
            pushButtons[opt]->setText(QString::number(val / factor, 'g', 5));
            if (!cam_option_changed_timer.isActive()) cam_option_changed_timer.start(100);
            });
        if (opt_range.support_type == capture::OPTION_AUTO) {

            connect(checkboxs[opt], &QCheckBox::toggled, this, [pushButtons,sliders, opt, this, &cam_option_changed, factor](bool status) {
                cam_option_changed |= (1 << opt);
                sliders[opt]->setEnabled(status);
                if (!status)
                    pushButtons[opt]->setText("AUTO");
                else
                    pushButtons[opt]->setText(QString::number(sliders[opt]->value() / factor, 'g', 5));
                if (!cam_option_changed_timer.isActive()) cam_option_changed_timer.start(100);
                });
        }
        else {
            checkboxs[opt]->setDisabled(true);
            connect(checkboxs[opt], &QCheckBox::toggled, this, [checkboxs, opt](bool status) {
                checkboxs[opt]->setChecked(true); });
        }
        connect(pushButtons[opt], &QPushButton::clicked, this, [=]() {
            if (reset_prop.status_type != capture::OPTION_INVALID) {
                if (sliders[opt]->value() == reset_prop.value)
                    emit sliders[opt]->valueChanged(reset_prop.value);
                else
                    sliders[opt]->setValue(reset_prop.value);
                if (reset_prop.status_type == capture::OPTION_AUTO) {
                    if (checkboxs[opt]->isChecked() == false)
                        emit checkboxs[opt]->toggled(false);
                    else
                        checkboxs[opt]->setChecked(false);
                }
                else {
                    pushButtons[opt]->setText(QString::number(reset_prop.value / factor, 'g', 5));
                    if (checkboxs[opt]->isChecked() == true)
                        emit checkboxs[opt]->toggled(true);
                    else
                        checkboxs[opt]->setChecked(true);
                }
            }
            });
    }
    else {
        camopt_slider[opt]->setMinimum(0);
        camopt_slider[opt]->setMaximum(0);
        camopt_slider[opt]->setValue(0);
        camopt_slider[opt]->setDisabled(true);
        camopt_checkBox[opt]->setDisabled(true);
        camopt_checkBox[opt]->setChecked(true);
        camopt_pushButton[opt]->setDisabled(true);
        camopt_pushButton[opt]->setText("NA");
    }
}
void VideoUI::loadCameraOptions(capture::CameraStream* stream) {
    // device spec
    capture::CameraDevice* device = stream->device;
    for (int opt = 0; opt < capture::DEVICE_OPTION_CNT; opt++) {
        auto opt_range = device->get_option_range((capture::DEVICE_OPTION)opt);

        if (opt == capture::DEVICE_BACKLIGHT
            || opt == capture::DEVICE_COLOR_ENABLED) {
            camopt_checkBox[opt]->disconnect(this);
            camopt_pushButton[opt]->disconnect(this);
            if (opt_range.is_supported) {
                camopt_checkBox[opt]->setDisabled(false);
                camopt_pushButton[opt]->setDisabled(false);
                auto current_opt = device->get_option((capture::DEVICE_OPTION)opt);
                if (current_opt.value == 0)
                {
                    camopt_checkBox[opt]->setChecked(false);
                    camopt_pushButton[opt]->setText("DIS");

                }
                else
                {
                    camopt_checkBox[opt]->setChecked(true);
                    camopt_pushButton[opt]->setText("EN");
                }
                connect(camopt_checkBox[opt], &QCheckBox::toggled, this, [this, opt](bool status) {
                    cam_option_changed |= (1 << opt);
                    if (!status)
                        camopt_pushButton[opt]->setText("DIS");
                    else
                        camopt_pushButton[opt]->setText("EN");

                    if (!cam_option_changed_timer.isActive()) cam_option_changed_timer.start(100);
                    });
                connect(camopt_pushButton[opt], &QPushButton::clicked, this, [this, opt, device]() {
                    auto reset_prop = device->get_reset_option((capture::DEVICE_OPTION)opt);
                    if (reset_prop.status_type != capture::OPTION_INVALID) {
                        if ((reset_prop.value == 1) == camopt_checkBox[opt]->isChecked())
                            emit camopt_checkBox[opt]->toggled(reset_prop.value == 1);
                        else {
                            camopt_checkBox[opt]->setChecked(reset_prop.value == 1);
                        }
                    }
                    });
            }
            else {
                camopt_checkBox[opt]->setDisabled(true);
                camopt_checkBox[opt]->setChecked(true);
                camopt_pushButton[opt]->setDisabled(true);
                camopt_pushButton[opt]->setText("NA");
            }
            continue;
        }
        connect_option_set(opt, camopt_slider, camopt_checkBox, camopt_pushButton, device, cam_option_changed);
    }

    // stream spec
    ui->slider_quality->setValue(capture->selected_stream->encoder_quality);
    ui->comboBox_codec->setCurrentIndex(
        ui->comboBox_codec->findText(QString::fromStdString(
            GET_PIX_TYPE_NAME(capture->selected_stream->encoder_method))));

    // stream transform
    for (int opt = 0; opt < capture::STREAM_LOSSY_OPTION_CNT; opt++) {
        capture::option_range current_transform = stream->get_option_range(opt);
        connect_option_set(opt, streamopt_slider, streamopt_checkBox, streamopt_pushButton, stream,stream_option_changed);
    }
    ui->pushButton_process_mode->disconnect(this);

    bool is_lossless = (stream->get_option(capture::STREAM_MODE).value == STREAM_OPTION_LOSSLESS);
    int rotate_mode = stream->get_option(capture::STREAM_ROTATE).value;

    if(is_lossless){
        ui->pushButton_process_mode->setText("Lossless");
        ui->lossy_grid->setEnabled(false);
    }
    else {
        ui->pushButton_process_mode->setText("Lossy");
        ui->lossy_grid->setEnabled(true);
    }
	connect(ui->pushButton_process_mode, &QPushButton::clicked, this, [this]() {
        if (ui->pushButton_process_mode->text() == "Lossless") {
            ui->pushButton_process_mode->setText("Lossy");
            ui->slider_process_rot_slider->setMaximum(360);
            ui->slider_process_rot_slider->setValue(ui->slider_process_rot_slider->value() * 90);
            ui->lossy_grid->setEnabled(true);
        }
        else {
            ui->pushButton_process_mode->setText("Lossless");
            ui->slider_process_rot_slider->setMaximum(3);
            ui->slider_process_rot_slider->setValue(0);
            ui->lossy_grid->setEnabled(false);
        }
        stream_option_changed |= (1 << capture::STREAM_MODE);
        if (!cam_option_changed_timer.isActive()) cam_option_changed_timer.start(100);
        });

        ui->checkBox_process_fliplr->disconnect(this);
        if (stream->get_option(capture::STREAM_FLIP_LR).value)
            ui->checkBox_process_fliplr->setChecked(true);
        else
            ui->checkBox_process_fliplr->setChecked(false);
        connect(ui->checkBox_process_fliplr, &QCheckBox::toggled, this, [this](bool checked) {
            stream_option_changed |= (1 << capture::STREAM_FLIP_LR);
            if (!cam_option_changed_timer.isActive()) cam_option_changed_timer.start(100);
            });
        ui->checkBox_process_flipud->disconnect(this);
        if (stream->get_option(capture::STREAM_FLIP_UD).value)
            ui->checkBox_process_flipud->setChecked(true);
        else
            ui->checkBox_process_flipud->setChecked(false);
        connect(ui->checkBox_process_flipud, &QCheckBox::toggled, this, [this](bool checked) {
            stream_option_changed |= (1 << capture::STREAM_FLIP_UD);
            if (!cam_option_changed_timer.isActive()) cam_option_changed_timer.start(100);
            });
        ui->slider_process_rot_slider->disconnect(this);
        ui->pushButton_process_rot->disconnect(this);

        ui->slider_process_rot_slider->setValue(rotate_mode);
        ui->pushButton_process_rot->setText(QString::number(rotate_mode) + "\xc2\xb0");
        connect(ui->slider_process_rot_slider, &QSlider::valueChanged, this, [this](int value) {
            if (ui->pushButton_process_mode->text() == "Lossless")
                ui->pushButton_process_rot->setText(QString::number(ui->slider_process_rot_slider->value() * 90) + "\xc2\xb0");
            else
                ui->pushButton_process_rot->setText(QString::number(ui->slider_process_rot_slider->value()) + "\xc2\xb0");
            stream_option_changed |= (1 << capture::STREAM_ROTATE);
            if (!cam_option_changed_timer.isActive()) cam_option_changed_timer.start(100);
            });
        connect(ui->pushButton_process_rot, &QPushButton::clicked, this, [this](bool checked) {
            ui->slider_process_rot_slider->setValue(0);
            });


        // set roi
        ui->lineEdit_crop_roi->disconnect(this);
        ui->pushButton_crop_roi->disconnect(this);

        connect(ui->pushButton_crop_roi, &QPushButton::toggled, this, [this](bool is_checked) {
            if (is_checked) {
                ui->tab_postprocess->setEnabled(false);
            }
            });
        connect(ui->pushButton_reset_roi, &QPushButton::clicked, this, [this]() {
            emit ui->lineEdit_crop_roi->rightClicked();
            });


        unsigned x = (unsigned)stream->get_option_value(capture::STREAM_CROP_X);
        unsigned y = (unsigned)stream->get_option_value(capture::STREAM_CROP_Y);
        unsigned w = (unsigned)stream->get_option_value(capture::STREAM_CROP_WIDTH);
        unsigned h = (unsigned)stream->get_option_value(capture::STREAM_CROP_HEIGHT);
        ui->lineEdit_crop_roi->setProperty("roi_x", QVariant::fromValue(x));
        ui->lineEdit_crop_roi->setProperty("roi_y", QVariant::fromValue(y));
        ui->lineEdit_crop_roi->setProperty("roi_w", QVariant::fromValue(w));
        ui->lineEdit_crop_roi->setProperty("roi_h", QVariant::fromValue(h));
		ui->lineEdit_crop_roi->setText(QString::asprintf("(%d,%d,%d,%d)", x, y, w, h));
        Resolution  resol = stream->get_current_profile()->resolution;

        //connect(ui->lineEdit_crop_roi, &QLineEdit::, this, [this, resol]() {
        connect(ui->lineEdit_crop_roi, &QLineEdit::editingFinished, this, [this, resol]() {
            QRegularExpression regex("\\((\\d+),(\\d+),(\\d+),(\\d+)\\)");
            QRegularExpressionMatch match = regex.match(ui->lineEdit_crop_roi->text());
            if (match.hasMatch()) {
                unsigned x = match.captured(1).toUInt();
                unsigned y = match.captured(2).toUInt();
                unsigned w = match.captured(3).toUInt();
                unsigned h = match.captured(4).toUInt();
                if (x > resol.width) x = 0;
                if (y > resol.height) y = 0;
                if (x+w > resol.width) w = resol.width- x;
                if (y > resol.height) y = resol.height - y;

                ui->lineEdit_crop_roi->setText(QString::asprintf("(%d,%d,%d,%d)", x, y, w, h));
                ui->lineEdit_crop_roi->setProperty("roi_x", QVariant::fromValue(x));
                ui->lineEdit_crop_roi->setProperty("roi_y", QVariant::fromValue(y));
                ui->lineEdit_crop_roi->setProperty("roi_w", QVariant::fromValue(w));
                ui->lineEdit_crop_roi->setProperty("roi_h", QVariant::fromValue(h));
            }
            else {
                ui->lineEdit_crop_roi->setText(QString::asprintf("(%d,%d,%d,%d)", 0, 0, resol.width, resol.height));
                ui->lineEdit_crop_roi->setProperty("roi_x", QVariant::fromValue(0));
                ui->lineEdit_crop_roi->setProperty("roi_y", QVariant::fromValue(0));
                ui->lineEdit_crop_roi->setProperty("roi_w", QVariant::fromValue(resol.width));
                ui->lineEdit_crop_roi->setProperty("roi_h", QVariant::fromValue(resol.height));
            }
            stream_option_changed |= (1 << capture::STREAM_CROP_X);
            if (!cam_option_changed_timer.isActive()) cam_option_changed_timer.start(100);
        });
        connect(ui->lineEdit_crop_roi, &SilentLineEdit::rightClicked,this, [this, resol]() {
            ui->lineEdit_crop_roi->setText(QString::asprintf("(%d,%d,%d,%d)", 0, 0, resol.width, resol.height));
            ui->lineEdit_crop_roi->setProperty("roi_x", QVariant::fromValue(0));
            ui->lineEdit_crop_roi->setProperty("roi_y", QVariant::fromValue(0));
            ui->lineEdit_crop_roi->setProperty("roi_w", QVariant::fromValue(resol.width));
            ui->lineEdit_crop_roi->setProperty("roi_h", QVariant::fromValue(resol.height));
            stream_option_changed |= (1 << capture::STREAM_CROP_X);
            if (!cam_option_changed_timer.isActive()) cam_option_changed_timer.start(100);
            });

}

void VideoUI::set_sensor_property() {
    auto stream = capture->selected_stream;
    auto device = stream->device;
    for (int opt = 0; opt < capture::DEVICE_OPTION_CNT; opt++) {
        if ((cam_option_changed & (1 << opt)) != 0) {
            if (opt == capture::DEVICE_BACKLIGHT
                || opt == capture::DEVICE_COLOR_ENABLED) {
                device->set_option((capture::DEVICE_OPTION)opt,
                    { camopt_checkBox[opt]->isChecked() ? 1 : 0,capture::OPTION_MANUAL });
                continue;
            }
            if (camopt_checkBox[opt]->isChecked()) {
                device->set_option((capture::DEVICE_OPTION)opt,
                    { camopt_slider[opt]->value(),capture::OPTION_MANUAL });
            }
            else {
                device->set_option((capture::DEVICE_OPTION)opt,
                    { 0,capture::OPTION_AUTO });
            }
        }
    }
    cam_option_changed = 0;
    bool need_enable = false;
    for (int opt = 0; opt < capture::STREAM_LOSSY_OPTION_CNT; opt++) {

        if ((stream_option_changed & (1 << opt)) != 0) {
            if (streamopt_checkBox[opt]->isChecked()) {
                stream->set_option((capture::STREAM_OPTION)opt,
                    { streamopt_slider[opt]->value(),capture::OPTION_MANUAL });
            }
            else {
                stream->set_option((capture::STREAM_OPTION)opt,
                    { 0,capture::OPTION_AUTO });
            }
        }

        need_enable = (!stream->is_default(opt));
    }



    if (stream->get_option(capture::STREAM_MODE).value == STREAM_OPTION_LOSSLESS)need_enable = false;

    if ((stream_option_changed & (1 << capture::STREAM_FLIP_LR)) != 0) {
		if (ui->checkBox_process_fliplr->isChecked())
            stream->set_option(capture::STREAM_FLIP_LR, { 1,capture::OPTION_MANUAL });
		else
            stream->set_option(capture::STREAM_FLIP_LR, { 0,capture::OPTION_MANUAL });
    }
    need_enable = (!stream->is_default(capture::STREAM_FLIP_LR));

    if ((stream_option_changed & (1 << capture::STREAM_FLIP_UD)) != 0) {
        if (ui->checkBox_process_flipud->isChecked())
            stream->set_option(capture::STREAM_FLIP_UD, { 1,capture::OPTION_MANUAL });
        else
            stream->set_option(capture::STREAM_FLIP_UD, { 0,capture::OPTION_MANUAL });
    }
    need_enable = (!stream->is_default(capture::STREAM_FLIP_UD));

    if ((stream_option_changed & (1 << capture::STREAM_ROTATE)) != 0) {
        if (ui->pushButton_process_mode->text() == "Lossless")
            stream->set_option(capture::STREAM_ROTATE, { ui->slider_process_rot_slider->value() * 90,capture::OPTION_MANUAL });
        else
            stream->set_option(capture::STREAM_ROTATE, { ui->slider_process_rot_slider->value() ,capture::OPTION_MANUAL });
    }
    need_enable = (!stream->is_default(capture::STREAM_ROTATE));


    if ((stream_option_changed & (1 << capture::STREAM_CROP_X)) != 0) {
        unsigned x = ui->lineEdit_crop_roi->property("roi_x").toUInt();
        unsigned y = ui->lineEdit_crop_roi->property("roi_y").toUInt();
        unsigned w = ui->lineEdit_crop_roi->property("roi_w").toUInt();
        unsigned h = ui->lineEdit_crop_roi->property("roi_h").toUInt();
        stream->set_value(capture::STREAM_CROP_X, x);
        stream->set_value(capture::STREAM_CROP_Y, y);
        stream->set_value(capture::STREAM_CROP_WIDTH, w);
        stream->set_value(capture::STREAM_CROP_HEIGHT, h);
    }
    need_enable = (!stream->is_default(capture::STREAM_CROP_X));
    need_enable = (!stream->is_default(capture::STREAM_CROP_Y));
    need_enable = (!stream->is_default(capture::STREAM_CROP_WIDTH));
    need_enable = (!stream->is_default(capture::STREAM_CROP_HEIGHT));

    if ((stream_option_changed & (1 << capture::STREAM_MODE)) != 0) {
        if (ui->pushButton_process_mode->text() == "Lossless") {
            stream->set_option(capture::STREAM_MODE, { STREAM_OPTION_LOSSLESS,need_enable?capture::OPTION_MANUAL: capture::OPTION_AUTO });
        }
        else {
            stream->set_option(capture::STREAM_MODE, { STREAM_OPTION_LOSSY,need_enable ? capture::OPTION_MANUAL : capture::OPTION_AUTO });
        }
    }

    stream_option_changed = 0;
}
template <typename T>
std::set<T> intersectSets(const std::vector<std::set<T>>& sets) {
    if (sets.empty()) {
        return {};  // Return an empty set if no sets are provided
    }

    // Start with the first set
    std::set<T> intersection(sets[0].begin(), sets[0].end());

    // Iterate over the remaining sets and compute the intersection
    for (size_t i = 1; i < sets.size(); ++i) {
        std::set<T> tempIntersection;
        std::set_intersection(intersection.begin(), intersection.end(),
            sets[i].begin(), sets[i].end(),
            std::inserter(tempIntersection, tempIntersection.end()));
        intersection = tempIntersection;
    }

    return intersection;
}
void VideoUI::encoder_changed() {
    capture::CameraStream* stream = comboBox_stream->getHighLightData().value<capture::CameraStream*>();
    if (ui->comboBox_codec->currentText().toStdString() !=  GET_PIX_TYPE_NAME(stream->encoder_method)) {
        std::vector<std::set<PIX_TYPE>> support_pixs;
        support_pixs.push_back(MediaWriter::get_supported_encoders(stream->get_current_profile()->format));

        auto support_pix = intersectSets(support_pixs);
        std::string pix_val = ui->comboBox_codec->currentText().toStdString();
        if (support_pix.find(
            (PIX_TYPE)PIX_FOURCC_TO_UINT32(pix_val))
            == support_pix.end()) {
            ui->comboBox_codec->setCurrentIndex(
                ui->comboBox_codec->findText(QString::fromStdString("MJPG"))); //default
            return;
        }
    }
    ui->slider_quality->disconnect(this);
    if (ui->comboBox_codec->currentText() == "MJPG") {
        stream->encoder_method = PIX_TYPE_MJPG;
        ui->slider_quality->setMaximum(100);
        ui->slider_quality->setMinimum(0);
        ui->compress_label->setText("Quality");
        connect(ui->slider_quality, &QSlider::valueChanged, this, [this, stream](int val) {
            stream->encoder_quality = val;
            if (val == 100)ui->label_quality->setText("MAX");
            else
                ui->label_quality->setText(QString::number(val) + "%");
            });
        ui->slider_quality->setValue(stream->encoder_quality);
    }
    else if (ui->comboBox_codec->currentText() == "HFYU") {
        stream->encoder_method = PIX_TYPE_HFYU;
        ui->slider_quality->setMaximum(0);
        ui->slider_quality->setMinimum(0);
        ui->slider_quality->setValue(0);
        ui->compress_label->setText("Huffman");
        ui->label_quality->setText("NA");
    }
    else if (ui->comboBox_codec->currentText() == "RAW ") {
        stream->encoder_method = PIX_TYPE_RAW;
        ui->slider_quality->setMaximum(0);
        ui->slider_quality->setMinimum(0);
        ui->slider_quality->setValue(0);
        ui->compress_label->setText("RawRGB");
        ui->label_quality->setText(QString::number(0));
    }
}




void VideoUI::onActionRefreshCamera() {
    setCursorBusy(true);
    actionrefreshCamera->setDisabled(true);
    comboBox_cameras->clear();
    comboBox_cameras->setCurrentIndex(-1);
    comboBox_profile->clear();
    comboBox_profile->setCurrentIndex(-1);
    comboBox_stream->clear();
    comboBox_stream->setCurrentIndex(-1);
    comboBox_profile_type->clear();
    comboBox_profile_type->setCurrentIndex(-1);
    comboBox_cameras->setDisabled(true);
    actionStartCamera->setDisabled(true);
    lock_camera_info_play(false);
    static capture::devices_set_t current_devices;
    worker.run_with_call_back(
        []() {capture::refresh_devices(current_devices); },
        [this]() {
            for (auto& device : current_devices) {
                comboBox_cameras->addItem(QString::fromStdString(device->device_name), QVariant::fromValue(device), device->is_running());
            }
            comboBox_cameras->setCurrentIndex(-1);
            comboBox_cameras->setDisabled(false);
            actionrefreshCamera->setDisabled(false);
            setCursorBusy(false);
        }
    );
}


void VideoUI::onActionStartCamera() {
    setCursorBusy(true);
    actionStartCamera->setDisabled(true);
    int device_idx = comboBox_cameras->getHighLight();
    capture::CameraDevice* device = comboBox_cameras->getHighLightData().value<capture::CameraDevice*>();

    if (comboBox_cameras->isSelected(device_idx)) {
        // camera is running
        int prof_idx = comboBox_profile->currentIndex();
        if (prof_idx != -1) {
            // stream selected
            capture::CameraStream* stream = comboBox_profile->itemData(prof_idx).value<capture::CameraProfile*>()->stream;
            if (device->is_stream_enabled(stream)) {
                switchStreamStatus(stream, false);
            }
            else {
                switchStreamStatus(stream, true);
            }
        }
        else {
            // no profile select
            switchCameraStatus(device, false);
        }
    }
    else {
        // camera not running
        int prof_idx = comboBox_profile->currentIndex();
        if (prof_idx != -1) {
            capture::CameraStream* stream = comboBox_profile->itemData(prof_idx).value<capture::CameraProfile*>()->stream;
            switchStreamStatus(stream, true);
        }
        else {
            // invalid status
        }

    }
    setCursorBusy(false);
}
