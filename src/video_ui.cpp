#include "video_ui.h"
#include "MultiSelectComboBox.h"
#include <converter.h>
#include <capture.h>
#include <RPPGExtractor.h>
#include "./ui_mainwindow.h"
#include "signalprocess.h"
VideoUI::VideoUI(Ui::MainWindow* ui, SignalProcess* signalProcess, QWidget* parent)
    :QWidget(parent), ui(ui), worker(this)
{
    setVisible(false);
    ui->actionStartCamera->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    ui->actionrefreshCamera->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));

    comboBox_cameras = new MultiSelectComboBox(ui->toolBarCamera);
    comboBox_stream = new MultiSelectComboBox(ui->toolBarCamera);
    comboBox_profile_type = new QComboBox(ui->toolBarCamera);
    comboBox_profile = new QComboBox(ui->toolBarCamera);
    comboBox_cameras->setFixedHeight(30);
    comboBox_cameras->setFixedWidth(225);
    comboBox_stream->setFixedWidth(110);
    comboBox_profile_type->setFixedWidth(60);
    comboBox_profile->setFixedWidth(130);
    ui->toolBarCamera->addWidget(comboBox_cameras);
    ui->toolBarCamera->addWidget(comboBox_stream);
    ui->toolBarCamera->addWidget(comboBox_profile_type);
    ui->toolBarCamera->addWidget(comboBox_profile);
    connect(comboBox_cameras, &MultiSelectComboBox::highLightSelect, this, &VideoUI::onCameraHighLighted);
    connect(comboBox_cameras, &MultiSelectComboBox::selectionChanged, this, &VideoUI::onCameraSelected);
    connect(comboBox_stream, &MultiSelectComboBox::highLightSelect, this, &VideoUI::onStreamHighLighted);
    connect(comboBox_stream, &MultiSelectComboBox::selectionChanged, this, &VideoUI::onStreamSelected);
    connect(comboBox_profile_type, &QComboBox::activated, this, &VideoUI::onProfileTypeSelected);
    connect(comboBox_profile, &QComboBox::activated, this, &VideoUI::onProfileSelected);
    
    //connect(ui->actionStartCamera, &QAction::triggered, this, &VideoUI::onActionStartCamera);
    //connect(ui->actionrefreshCamera, &QAction::triggered, this, &VideoUI::onActionRefreshCamera);


    textedit_streamname = new QLineEdit(ui->toolBarCamera);
    textedit_streamname->setMaximumWidth(200);
    textedit_streamname->setDisabled(true);
    textedit_streamname->setPlaceholderText("Device Filename");
    ui->toolBarCamera->addWidget(textedit_streamname);


    //onActionRefreshCamera();


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

    connect(converter, &Converter::set_roi, this, [this](cv::Rect rect) {
        if (rect.area() == 0) {
            this->ui->actionTracking->setText("Untracked");
            this->ui->actionTracking->setChecked(false);
            face_tracking->disable_tracking();
        }
        else {
            this->ui->actionTracking->setText("ROI");
            this->ui->actionTracking->setChecked(true);
            face_tracking->set_roi(rect);
        }
        });

    connect(face_tracking, &ColorExtractor::on_signal_ready, signalProcess, &SignalProcess::processSignal);
    connect(face_tracking, &ColorExtractor::on_face_lost, signalProcess, &SignalProcess::reset_rppg);

    connect(ui->actionTracking, &QPushButton::clicked, this, [this](bool checked) {
        if (checked) {
            this->ui->actionTracking->setText("Tracking");
            face_tracking->set_roi({ 0,0,0,0 });
        }
        else {
            this->ui->actionTracking->setText("Untracked");
            face_tracking->disable_tracking();
        }
        });

    cam_option_changed_timer.setSingleShot(false);
#define match_array_qobj_camopts(obj,dev,name) camopt_##obj[capture::CameraDevice::DEVICE_##dev] = ui->obj##_##name

#define  match_array_camopts(obj) \
    match_array_qobj_camopts(##obj, EXPOSURE, exposure);\
    match_array_qobj_camopts(##obj, GAIN, gain);\
    match_array_qobj_camopts(##obj, WHITE_BALANCE, whiteBalance);\
    match_array_qobj_camopts(##obj, GAMMA, gamma);\
    match_array_qobj_camopts(##obj, LIGHT, light);\
    match_array_qobj_camopts(##obj, ZOOM, zoom);\
    match_array_qobj_camopts(##obj, PAN, pan);\
    match_array_qobj_camopts(##obj, TILT, tilt);\
    match_array_qobj_camopts(##obj, ROLL, roll);\
    match_array_qobj_camopts(##obj, IRIS, iris);\
    match_array_qobj_camopts(##obj, FOCUS, focus);\
    match_array_qobj_camopts(##obj, CONTRAST, contrast);\
    match_array_qobj_camopts(##obj, HUE, hue);\
    match_array_qobj_camopts(##obj, SATURATION, saturation);\
    match_array_qobj_camopts(##obj, SHARPNESS, sharpness);\
    match_array_qobj_camopts(##obj, BRIGHTNESS, brightness);

    match_array_camopts(checkBox);
    match_array_camopts(pushButton);
    match_array_camopts(slider);


    match_array_qobj_camopts(checkBox, BACKLIGHT, backlight);
    match_array_qobj_camopts(checkBox, COLOR_ENABLED, colorEnabled);
    match_array_qobj_camopts(pushButton, BACKLIGHT, backlight);
    match_array_qobj_camopts(pushButton, COLOR_ENABLED, colorEnabled);

    connect(ui->pushButton_resetVideo, &QPushButton::clicked, this, [this]() {
        for (int opt = 8; opt < capture::CameraDevice::DEVICE_OPTION_CNT; opt++) {
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

    connect(&cam_option_changed_timer, &QTimer::timeout, this, [this]() {
        cam_option_changed_timer.stop();
        set_sensor_property();
        });


    connect(ui->scroll_rotation, &QScrollBar::valueChanged, this, [this](int val) {
        capture->rot = val;
        this->ui->label_rotation->setText(QString::number(val) + "\xc2\xb0");
        });


    connect(ui->comboBox_codec, &QComboBox::currentIndexChanged, this, &VideoUI::encoder_changed);

    connect(ui->button_rotation, &QPushButton::clicked, this, [this]() {
        this->ui->scroll_rotation->setValue(0);
        });
    connect(ui->checkBox_fliplr, &QCheckBox::toggled, this, [this](bool checked) {
        capture->is_fliplr = checked;
        });
    connect(ui->checkBox_flipud, &QCheckBox::toggled, this, [this](bool checked) {
        capture->is_flipud = checked;
        });

    ui->VideoBox->hide();
    ui->VideoBox->setDisabled(true);

    capture->start();

    worker.wait();
}

VideoUI::~VideoUI()
{
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
                        auto v_rec = new MediaWriter(f_name,
                            stream->selected_profile->resolution, stream->selected_profile->ratio, stream->selected_profile->format,
                            stream->encoder_method, stream->encoder_quality);
                        rec_maps[stream] = v_rec;
                    }
                }
            }
        },
        [this]() {
            recorder_lock.lock();
            is_recording = true;
            recorder_lock.unlock();
            setCursorBusy(false);
        }
    );
}

void VideoUI::stop_record()
{
    setCursorBusy(true);

    worker.run_with_call_back(
        [this]() {
            for (auto& [stream, rec] : rec_maps) delete rec;
            rec_maps.clear();
        },
        [this]() {
            recorder_lock.lock();
            is_recording = false;
            recorder_lock.unlock();
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
        ui->actionStartCamera->setToolTip("Stop");
        ui->actionStartCamera->setIcon(style()->standardIcon(QStyle::SP_MediaStop));

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
        ui->actionStartCamera->setToolTip("Play");
        ui->actionStartCamera->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));

        ui->VideoBox->hide();
        textedit_streamname->disconnect(this);
        textedit_streamname->setDisabled(true);
        textedit_streamname->setText("");
    }
}


void VideoUI::onDeviceSelected(capture::CameraStream* stream) {
    if (stream == nullptr) {
        comboBox_cameras->setHighLight(-1, false);
        capture->selected_stream = nullptr;
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
    }
}
void VideoUI::onCameraSelected(int idx, bool is_selected) {
    comboBox_stream->clear();
    comboBox_profile_type->clear();
    comboBox_profile->clear();
    ui->actionStartCamera->setDisabled(true);

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
            });
    }
    else {
        switchCameraStatus(device, false);
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
                ui->actionStartCamera->setDisabled(false);
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
        ui->actionStartCamera->setDisabled(true);
        setCursorBusy(false);
    }
    else {
        comboBox_stream->clear();
        comboBox_profile_type->clear();
        comboBox_profile->clear();
        ui->actionStartCamera->setDisabled(true);
        lock_camera_info_play(false);
        capture->selected_stream = nullptr;
        return;
    }
}
void VideoUI::onStreamSelected(int idx, bool is_selected) {
    capture::CameraStream* stream = comboBox_stream->itemData(idx).value<capture::CameraStream*>();

    if (is_selected) {
        LoadStreamProfiles(stream);
        if (comboBox_stream->getSelectedItems().size() != 0)
            ui->actionStartCamera->setDisabled(false);
        else
            ui->actionStartCamera->setDisabled(true);
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
    auto profile_type_name = stream->selected_profile->get_profile_codec();
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
        if (stream->device->is_stream_enabled(stream)) {
            LoadStreamProfiles(stream);
            capture->selected_stream = stream;
            converter->set_roi({ 0,0,0,0 });
            lock_camera_info_play(true);
            ui->actionStartCamera->setDisabled(false);
            loadCameraOptions(stream);
        }
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
        if (*(profile->stream->selected_profile) == *(profile)) {
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

    ui->actionStartCamera->setDisabled(false);
    ui->toolBarCamera->setDisabled(false);
    //setCursorBusy(false);
}

void VideoUI::loadCameraOptions(capture::CameraStream* stream) {

    capture::CameraDevice* device = stream->device;
    ui->slider_quality->setValue(capture->selected_stream->encoder_quality);
    for (int opt = 0; opt < capture::CameraDevice::DEVICE_OPTION_CNT; opt++) {
        auto opt_range = device->get_option_range((capture::CameraDevice::DEVICE_OPTION)opt);

        if (opt == capture::CameraDevice::DEVICE_BACKLIGHT
            || opt == capture::CameraDevice::DEVICE_COLOR_ENABLED) {
            camopt_checkBox[opt]->disconnect(this);
            camopt_pushButton[opt]->disconnect(this);
            if (opt_range.is_supported) {
                camopt_checkBox[opt]->setDisabled(false);
                camopt_pushButton[opt]->setDisabled(false);
                auto current_opt = device->get_option((capture::CameraDevice::DEVICE_OPTION)opt);
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
                    auto reset_prop = device->get_reset_option((capture::CameraDevice::DEVICE_OPTION)opt);
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
        camopt_slider[opt]->disconnect(this);
        camopt_checkBox[opt]->disconnect(this);
        camopt_pushButton[opt]->disconnect(this);


        if (opt_range.is_supported) {
            camopt_slider[opt]->setDisabled(false);
            camopt_checkBox[opt]->setDisabled(false);
            camopt_pushButton[opt]->setDisabled(false);
            camopt_slider[opt]->setMinimum(opt_range.min);
            camopt_slider[opt]->setMaximum(opt_range.max);
            camopt_slider[opt]->setSingleStep(opt_range.step);
            auto current_opt = device->get_option((capture::CameraDevice::DEVICE_OPTION)opt);
            if (current_opt.status_type == capture::OPTION_AUTO) {
                camopt_checkBox[opt]->setChecked(false);
                camopt_pushButton[opt]->setText("AUTO");
                camopt_slider[opt]->setEnabled(false);
                camopt_slider[opt]->setValue(opt_range.def.value);
            }
            else {
                camopt_checkBox[opt]->setChecked(true);
                camopt_slider[opt]->setEnabled(true);
                camopt_slider[opt]->setValue(current_opt.value);
                camopt_pushButton[opt]->setText(QString::number(current_opt.value));
            }
            connect(camopt_slider[opt], &QSlider::valueChanged, this, [this, opt](int val) {
                cam_option_changed |= (1 << opt);
                camopt_pushButton[opt]->setText(QString::number(val));
                if (!cam_option_changed_timer.isActive()) cam_option_changed_timer.start(100);
                });
            if (opt_range.support_type == capture::OPTION_AUTO) {

                connect(camopt_checkBox[opt], &QCheckBox::toggled, this, [this, opt](bool status) {
                    cam_option_changed |= (1 << opt);
                    camopt_slider[opt]->setEnabled(status);
                    if (!status)
                        camopt_pushButton[opt]->setText("AUTO");
                    if (!cam_option_changed_timer.isActive()) cam_option_changed_timer.start(100);
                    });

            }
            else {

                connect(camopt_checkBox[opt], &QCheckBox::toggled, this, [this, opt](bool status) {
                    camopt_checkBox[opt]->setChecked(true); });
            }
            connect(camopt_pushButton[opt], &QPushButton::clicked, this, [this, opt, device]() {
                auto reset_prop = device->get_reset_option((capture::CameraDevice::DEVICE_OPTION)opt);
                if (reset_prop.status_type != capture::OPTION_INVALID) {
                    if (camopt_slider[opt]->value() == reset_prop.value)
                        emit camopt_slider[opt]->valueChanged(reset_prop.value);
                    else
                        camopt_slider[opt]->setValue(reset_prop.value);
                    if (reset_prop.status_type == capture::OPTION_AUTO) {
                        if (camopt_checkBox[opt]->isChecked() == false)
                            emit camopt_checkBox[opt]->toggled(false);
                        else
                            camopt_checkBox[opt]->setChecked(false);
                    }
                    else {
                        camopt_pushButton[opt]->setText(QString::number(reset_prop.value));
                        if (camopt_checkBox[opt]->isChecked() == true)
                            emit camopt_checkBox[opt]->toggled(true);
                        else
                            camopt_checkBox[opt]->setChecked(true);
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
    ui->comboBox_codec->setCurrentIndex(
        ui->comboBox_codec->findText(QString::fromStdString(
            GET_PIX_TYPE_NAME(capture->selected_stream->encoder_method))));
}

void VideoUI::set_sensor_property() {
    for (int opt = 0; opt < capture::CameraDevice::DEVICE_OPTION_CNT; opt++) {
        if ((cam_option_changed & (1 << opt)) != 0) {
            if (opt == capture::CameraDevice::DEVICE_BACKLIGHT
                || opt == capture::CameraDevice::DEVICE_COLOR_ENABLED) {
                capture->selected_stream->device->set_option((capture::CameraDevice::DEVICE_OPTION)opt,
                    { camopt_checkBox[opt]->isChecked() ? 1 : 0,capture::OPTION_MANUAL });
                continue;
            }
            if (camopt_checkBox[opt]->isChecked()) {
                capture->selected_stream->device->set_option((capture::CameraDevice::DEVICE_OPTION)opt,
                    { camopt_slider[opt]->value(),capture::OPTION_MANUAL });
            }
            else {
                capture->selected_stream->device->set_option((capture::CameraDevice::DEVICE_OPTION)opt,
                    { 0,capture::OPTION_AUTO });
            }
        }
    }
    cam_option_changed = 0;
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
    if (GET_PIX_TYPE_NAME(stream->encoder_method) != ui->comboBox_codec->currentText()) {
        std::vector<std::set<PIX_TYPE>> support_pixs;
        support_pixs.push_back(MediaWriter::get_supported_encoders(stream->selected_profile->format));

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
    ui->actionrefreshCamera->setDisabled(true);
    comboBox_cameras->clear();
    comboBox_cameras->setCurrentIndex(-1);
    comboBox_profile->clear();
    comboBox_profile->setCurrentIndex(-1);
    comboBox_stream->clear();
    comboBox_stream->setCurrentIndex(-1);
    comboBox_profile_type->clear();
    comboBox_profile_type->setCurrentIndex(-1);
    comboBox_cameras->setDisabled(true);
    ui->actionStartCamera->setDisabled(true);
    lock_camera_info_play(false);
    static capture::devices_set_t current_devices;
    worker.run_with_call_back(
        []() {capture::refresh_devices(current_devices); },
        [this]() {
            for (auto& device : current_devices) {
                comboBox_cameras->addItem(QString::fromStdString(device->device_name), QVariant::fromValue(device));
            }
            comboBox_cameras->setCurrentIndex(-1);
            comboBox_cameras->setDisabled(false);
            ui->actionrefreshCamera->setDisabled(false);
            setCursorBusy(false);
        }
    );
}


void VideoUI::onActionStartCamera() {
    setCursorBusy(true);
    ui->actionStartCamera->setDisabled(true);
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
