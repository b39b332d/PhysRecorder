#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "imageviewer.h"
#include<qdebug>
#include <QtWidgets>
#include <string>
#include <thread>
#include <QFile>
#include <QTextStream>
#include <qstandardpaths.h>

#include <RPPGExtractor.h>


void MainWindow::lock_camera_info_play(bool lock) {
    comboBox_profile_type->setDisabled(lock);
    comboBox_profile->setDisabled(lock);
    comboBox_stream->setDisabled(lock);
    if (lock) {
        ui->VideoBox->setDisabled(false);
        ui->actionStartTrigger->setToolTip("Stop");
        ui->actionStartTrigger->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    }
    else {
        ui->VideoBox->setDisabled(true);
        ui->actionStartTrigger->setToolTip("Play");
        ui->actionStartTrigger->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    }
}

void MainWindow::run_with_call_back(const std::function<void()>& run_in_thread, const std::function<void()>& call_back) {
    static QThread* th = NULL;
    if (th != NULL) {
        th->wait();
        th->deleteLater();
        th = NULL;
    }
    th = QThread::create(run_in_thread);
    connect(th, &QThread::finished, this, call_back);
    th->start();
}

void MainWindow::refresh_plot() {
    static uchar ref_cnt = 0;
    static bool ppgStatusFinished = true;
    static bool respiStatusFinished = true;
    static bool serialStatusFinished = true;
    if (ppg->isRunning() ){
        if (ppgStatusFinished) {
            ppgStatusFinished = false;
            ui->plot_ppg->graph(0)->addToLegend();
        }
    }
    else if(!ppgStatusFinished) {
        ppgStatusFinished = true;
        ui->plot_ppg->graph(0)->removeFromLegend();
    }
    if (respi->isRunning()){
        if (respiStatusFinished) {
            respiStatusFinished = false;
                ui->plot_ppg->graph(1)->addToLegend();
        }
    }
    else if (!respiStatusFinished) {
        respiStatusFinished = true;
        ui->plot_ppg->graph(1)->removeFromLegend();
    }
    if (custom_serial->isRunning()) {
        if (serialStatusFinished) {
            serialStatusFinished = false;
            ui->plot_ppg->graph(2)->addToLegend();
        }
    }
    else if (!serialStatusFinished) {
        serialStatusFinished = true;
        ui->plot_ppg->graph(2)->removeFromLegend();
    }
    double ts = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count() + 0.1;
    ui->plot_ppg->xAxis->setRange(ts, show_window_length, Qt::AlignRight);
    if (ref_cnt++ % 10 == 0) {
        ui->plot_ppg->graph(2)->data()->removeBefore(ts - show_window_length);
        ui->plot_ppg->graph(1)->data()->removeBefore(ts - show_window_length);
        ui->plot_ppg->graph(0)->data()->removeBefore(ts - show_window_length);


    }
    ui->plot_interp->xAxis->setRange(ts + signalProcess->cam_ofs, show_window_length, Qt::AlignRight);
    ui->plot_interp->xAxis2->setRange(ts, show_window_length, Qt::AlignRight);
    ui->plot_interp->yAxis->rescale();
    ui->plot_interp->yAxis2->rescale();
    ui->plot_ppg->yAxis2->rescale();
    ui->plot_interp->replot();
    ui->plot_ppg->replot();
}

void MainWindow::plotCustomSerial(uint32_t signal, double ts) {
    ui->plot_ppg->graph(2)->addData(ts, (double)signal / 0xFFFFFFFF);
    if (is_recording) {
        serial_ts_rec.push_back(ts);
        serial_sig_rec.push_back(signal);
    }
    
}
void MainWindow::plotPPG(uint16_t signal,double ts) {
    ui->plot_ppg->graph(0)->addData(ts, (double)signal/1024);
    //if (!refresh_plot_timer->isActive())
    //    refresh_plot_timer->start(20);
    if (is_recording) {
        ppg_ts_rec.push_back(ts);
        ppg_sig_rec.push_back(signal);
    }
    // rescale value (vertical) axis to fit the current data:
    //ui->plot_ppg->graph(0)->rescaleValueAxis();
}
void MainWindow::plotRESPI(uchar signal,double ts) {
    ui->plot_ppg->graph(1)->addData(ts, (double)signal/256);
    //if (!refresh_plot_timer->isActive())
    //    refresh_plot_timer->start(20);
    if (is_recording) {
        respi_ts_rec.push_back(ts);
        respi_sig_rec.push_back(signal);
    }
    // rescale value (vertical) axis to fit the current data:
    //ui->plot_ppg->graph(1)->rescaleValueAxis();
}

//void MainWindow::plotInterpPPG(double ppg, double ts) {
//
//}
//void MainWindow::plotInterpRGB(float r, float g, float b, float pos, const QVector<float>& pos_end, double ts) {
//    //qDebug() << r << g << b << ts << QThread::currentThreadId();
//    //if (sqi != NULL) {
//        //ui->label_SQIr->setText(QString::number(sqi[0], 'f', 2));
//        //ui->label_SQIg->setText(QString::number(sqi[1], 'f', 2));
//        //ui->label_SQIb->setText(QString::number(sqi[2], 'f', 2));
//        //ui->label_SNRr->setText(QString::number(sqi[3], 'f', 2));
//        //ui->label_SNRg->setText(QString::number(sqi[4], 'f', 2));
//        //ui->label_SNRb->setText(QString::number(sqi[5], 'f', 2));
//    //    delete[] sqi;
//    //}
//    //if (!refresh_plot_timer->isActive())
//    //    refresh_plot_timer->start(20);
//}
void MainWindow::on_device_selected(capture::CameraDevice* device) {
    if (device == nullptr) {
        comboBox_cameras->setCurrentIndex(-1); 
        onCameraSelected(-1);
    }
    else {
        int idx = comboBox_cameras->findData(QVariant::fromValue(device));
        if (idx != -1 ) {
            comboBox_cameras->setCurrentIndex(idx);
            onCameraSelected(idx);
        }
    }
}

void MainWindow::on_capture_device_disabled(capture::CameraDevice* device) {
    int idx = comboBox_cameras->findData(QVariant::fromValue(device));
    if (idx!=-1 && idx == comboBox_cameras->currentIndex()) {
        lock_camera_info_play(false);
    }
}
void MainWindow::onCameraSelected(int idx) {
    comboBox_stream->clear();
    comboBox_profile_type->clear();
    comboBox_profile->clear();
    ui->actionStartTrigger->setDisabled(true);

    if (idx == -1) {
        lock_camera_info_play(false);
        capture->selected_device = nullptr;
        ui->VideoBox->hide();
        return;
    }
    setCursor(Qt::WaitCursor);
    ui->VideoBox->show();
    capture::CameraDevice* device = comboBox_cameras->itemData(idx).value<capture::CameraDevice*>();

    capture->selected_device = device;
    emit converter->set_roi({ 0,0,0,0 });
    if (device->is_running()) {
        lock_camera_info_play(true);
        ui->actionStartTrigger->setDisabled(false);

        for (auto& [stream_name, stream] : device->streams_map) {
            if(device->enabled_streams.contains(stream))
                comboBox_stream->addItem(QString::fromStdString(stream_name), QVariant::fromValue(stream), true);
            else
                comboBox_stream->addItem(QString::fromStdString(stream_name), QVariant::fromValue(stream),false);
            
        }
        loadCameraOptions(device);
        onStreamHighted(-1, false);
        setCursor(Qt::ArrowCursor);
    }
    else {
        lock_camera_info_play(false);
        run_with_call_back([this, device]() {
            device->init();
            loadCameraOptions(device);
            },
            [this, device]() {
                for (auto& [stream_name, stream] : device->streams_map) {
            if (device->enabled_streams.contains(stream))
                        comboBox_stream->addItem(QString::fromStdString(stream_name), QVariant::fromValue(stream), true);
                    else
                        comboBox_stream->addItem(QString::fromStdString(stream_name), QVariant::fromValue(stream), false);
                }
                onStreamHighted(-1, false);
                ui->actionStartTrigger->setDisabled(true);
                setCursor(Qt::ArrowCursor);
            });
    }
}
void MainWindow::onStreamSelected(int idx,bool is_selected) {
    capture::CameraStream* stream = comboBox_stream->itemData(idx).value<capture::CameraStream*>();

    if (is_selected) {
        onStreamHighted(idx, true);
        stream->device->register_stream(stream->selected_profile);
        if(comboBox_stream->getSelectedItems().size()!=0)
            ui->actionStartTrigger->setDisabled(false);
        else
            ui->actionStartTrigger->setDisabled(true);
    }
    else {
        onStreamHighted(idx, false);
        stream->device->unregister_stream(stream);
    }
}
void MainWindow::onStreamHighted(int idx, bool is_highlight) {

    if (is_highlight) {
        capture::CameraStream* stream = comboBox_stream->itemData(idx).value<capture::CameraStream*>();

        comboBox_stream->setText(comboBox_stream->getItemText(idx));
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
    else {
        comboBox_stream->setText(QString("%1 Streams")
            .arg(comboBox_stream->getSelectedItems().size()));
        comboBox_profile_type->clear();
        comboBox_profile_type->setCurrentIndex(-1);
        comboBox_profile->clear();
        comboBox_profile->setCurrentIndex(-1);
    }
}

void MainWindow::onProfileTypeSelected(int idx) {
    setCursor(Qt::WaitCursor);
    comboBox_profile->clear();


    auto profiles = comboBox_profile_type->itemData(idx).value<std::set<capture::CameraProfile*, capture::CameraStream::Cmp>*>();
    int item_idx = 0;
    int select_idx = -1;
    for (auto& profile: *profiles) {
        if (*(profile->stream->selected_profile) == *(profile)) {
            select_idx = item_idx;
        }
        comboBox_profile->addItem(QString::fromStdString(profile->get_profile_str()),
            QVariant::fromValue(profile));
        item_idx++;
    }
    comboBox_profile->setCurrentIndex(select_idx);
    setCursor(Qt::ArrowCursor);
}

void MainWindow::onProfileSelected(int idx) {
    //setCursor(Qt::WaitCursor);
    //ui->toolBarRS->setDisabled(true);

    auto profile = comboBox_profile->itemData(idx).value<capture::CameraProfile*>();
    profile->stream->device->register_stream(profile);
    ui->actionStartTrigger->setDisabled(false);
    //ui->toolBarRS->setDisabled(false);
    //setCursor(Qt::ArrowCursor);
}
MainWindow::MainWindow(QSplashScreen& splash, QWidget* parent)
//use reference instead of pointer
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , ts_idx(-1)
    , zeropad('0')
    , totalSourceFileCanBeIndexed(-1)
    , setSource(false)
{
    splash.showMessage("Setting up UI");
    ui->setupUi(this);
    ////*** trial version ***////
    //Validation *val = new Validation();
    //val->start();

    //ui->statusIndicatorLabel->setStyleSheet("QLabel { color : cyan; }");

    this->setWindowTitle("Remote PhotoPlethysmoGraphy");

    comboBox_cameras = new QComboBox(ui->toolBarRS);
    comboBox_stream = new MultiSelectComboBox(ui->toolBarRS);
    comboBox_profile_type = new QComboBox(ui->toolBarRS);
    comboBox_profile = new QComboBox(ui->toolBarRS);
    comboBox_cameras->setFixedWidth(225);
    comboBox_stream->setFixedWidth(100);
    comboBox_profile_type->setFixedWidth(100);
    comboBox_profile->setFixedWidth(130);
    ui->toolBarRS->addWidget(comboBox_cameras);
    ui->toolBarRS->addWidget(comboBox_stream);
    ui->toolBarRS->addWidget(comboBox_profile_type);
    ui->toolBarRS->addWidget(comboBox_profile);
    connect(comboBox_cameras, &QComboBox::activated, this, &MainWindow::onCameraSelected);
    connect(comboBox_stream, &MultiSelectComboBox::heightSelect, this, &MainWindow::onStreamHighted);
    connect(comboBox_stream, &MultiSelectComboBox::selectionChanged, this, &MainWindow::onStreamSelected);
    connect(comboBox_profile_type, &QComboBox::activated, this, &MainWindow::onProfileTypeSelected);
    connect(comboBox_profile, &QComboBox::activated, this, &MainWindow::onProfileSelected);
    ui->actionStartTrigger->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    ui->actionRecord->setIcon(style()->standardIcon(QStyle::SP_DialogYesButton));
    ui->actionrefreshCamera->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    ui->actionrefreshSerial->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    ui->actionstopSerial->setIcon(style()->standardIcon(QStyle::SP_MediaStop));


    comboBox_serial = new QComboBox(ui->toolBarSD);
    comboBox_serial->setFixedWidth(150);
    ui->toolBarSD->insertWidget(ui->actionstopSerial, comboBox_serial);
    connect(comboBox_serial, &QComboBox::activated, this, &MainWindow::select_serial);


    auto labelname1 = new QLabel(ui->toolBarRC);
    labelname1->setText("time(s):");
    //labelname1->setFixedWidth(80);
    labelname1->setAttribute(Qt::WA_TranslucentBackground);
    ui->toolBarRC->addWidget(labelname1);

    spinRecordTime = new QSpinBox(ui->toolBarRC);
    spinRecordTime->setMaximum(99999);
    spinRecordTime->setMinimum(-1);
    spinRecordTime->setValue(-1);
    ui->toolBarRC->addWidget(spinRecordTime);

    auto labelname2 = new QLabel(ui->toolBarRC);
    labelname2->setText("filename:");
    labelname2->setAttribute(Qt::WA_TranslucentBackground);
    ui->toolBarRC->addWidget(labelname2);
    filenameLineEdit = new QLineEdit(ui->toolBarRC);
    ui->toolBarRC->addWidget(filenameLineEdit);


    record_timer = new QTimer(this);
    record_timer->setSingleShot(false);
    record_timer->setInterval(1000);
    connect(record_timer, &QTimer::timeout, this, [this]() {
        if (spinRecordTime->value() != -1) {
            spinRecordTime->setValue(spinRecordTime->value() - 1);
            if (spinRecordTime->value() == 0) {
                emitFileSignal(0);
                stop_record();
            }
        }
        });
    splash.showMessage("Detecting Realsense Device...");
    on_actionrefreshCamera_triggered();



    refresh_plot_timer = new QTimer(this);
    connect(refresh_plot_timer, &QTimer::timeout, this, &MainWindow::refresh_plot);
    QSharedPointer<QCPAxisTickerDateTime> timeTicker(new QCPAxisTickerDateTime);
    timeTicker->setDateTimeFormat("mm:ss");
    ui->plot_ppg->legend->setVisible(true);
    ui->plot_ppg->legend->setBrush(QBrush(QColor(255, 255, 255, 230)));

    ui->plot_ppg->xAxis->setTicker(timeTicker);
    ui->plot_ppg->yAxis->setRange(0, 1);
    ui->plot_ppg->addGraph();
    ui->plot_ppg->graph(0)->setPen(QPen(QColor(40, 110, 255)));
    ui->plot_ppg->graph(0)->setName("PPG");
    ui->plot_ppg->addGraph();
    ui->plot_ppg->graph(1)->setPen(QPen(QColor(255, 110, 40)));
    ui->plot_ppg->graph(1)->setName("RESPI");

    ui->plot_ppg->addGraph(ui->plot_ppg->xAxis, ui->plot_ppg->yAxis2);
    ui->plot_ppg->graph(2)->setName("SERIAL");
    ui->plot_ppg->graph(2)->setPen(QPen(QColor(10, 255, 40)));
    ui->plot_ppg->yAxis2->setVisible(false);
    ui->plot_ppg->graph(0)->removeFromLegend();
    ui->plot_ppg->graph(1)->removeFromLegend();
    ui->plot_ppg->graph(2)->removeFromLegend();

    signalProcess = new SignalProcess(ui->fftLabel);

    ui->plot_interp->xAxis2->setTicker(timeTicker);
    ui->plot_interp->addGraph();    
    signalProcess->graph_r = ui->plot_interp->graph(0);
    ui->plot_interp->addGraph();    
    signalProcess->graph_g = ui->plot_interp->graph(1);
    ui->plot_interp->addGraph(); 
    signalProcess->graph_b= ui->plot_interp->graph(2);
    ui->plot_interp->addGraph(); 
    signalProcess->graph_pos = ui->plot_interp->graph(3);
    ui->plot_interp->addGraph();
    signalProcess->graph_pos_end = ui->plot_interp->graph(4);
    ui->plot_interp->addGraph(ui->plot_interp->xAxis2, ui->plot_interp->yAxis2);
    signalProcess->graph_ppg = ui->plot_interp->graph(5);

    signalProcess->graph_r->setPen(QPen(QColorConstants::Red));
    signalProcess->graph_g->setPen(QPen(QColorConstants::Green));
    signalProcess->graph_b->setPen(QPen(QColorConstants::Blue));
    signalProcess->graph_pos->setPen(QPen(QColorConstants::Magenta));
    signalProcess->graph_pos_end->setPen(QPen(QColorConstants::Magenta));
    signalProcess->graph_ppg->setPen(QPen(QColorConstants::Cyan));
    // signalProcess thread takes control plot on graph algorithm, main thread used for refresh realtime

    ui->plot_interp->xAxis2->setVisible(true);
    ui->plot_interp->xAxis->setVisible(false);

    auto th2 = new QThread();
    signalProcess->moveToThread(th2);
    th2->start();

    splash.showMessage("Creating threads");
    converter = new Converter(ui->q_video);
    QThread* converterThread = new QThread();

    capture = new Capture(*converter, signalProcess);
    converter->moveToThread(converterThread);
    connect(capture, &Capture::device_disabled, this, &MainWindow::on_capture_device_disabled);
    connect(capture->signalProcess, &SignalProcess::fftReady, this, &MainWindow::setfft);
    auto o = QObject::connect(converter, &Converter::frameReady, ui->q_video, &ImageViewer::setImage);
    connect(converter, &Converter::device_selected, this, &MainWindow::on_device_selected);

    converterThread->start();

    splash.showMessage("Detecting Serial Device...");

    respi = new RESPIReader();
    connect(respi, &RESPIReader::respiReady, this, &MainWindow::plotRESPI);

    ppg = new PPGReader();
    connect(ppg, &PPGReader::ppgReady, this, &MainWindow::plotPPG);
    connect(ppg, &PPGReader::ppgReady, capture->signalProcess, &SignalProcess::processPPG);

    custom_serial = new CustomSerialReader();
    connect(custom_serial, &CustomSerialReader::serialReady, this, &MainWindow::plotCustomSerial);
    

    connect(custom_serial, &PPGReader::finished, this, [this] {
        comboBox_serial->setCurrentIndex(-1);
        });

    freshSerialDevices();

    ui->VideoBox->hide();



    splash.showMessage("Connect signals");
    connect(ui->actionR, &QPushButton::toggled,capture->signalProcess, &SignalProcess::setShowR);
    connect(ui->actionG, &QPushButton::toggled, capture->signalProcess, &SignalProcess::setShowG);
    connect(ui->actionB, &QPushButton::toggled, capture->signalProcess, &SignalProcess::setShowB);

    connect(capture, &Capture::signalReady, capture->signalProcess, &SignalProcess::processSignal);
    connect(ui->actionTracking, &QPushButton::clicked, this, [this](bool checked) {
        std::unique_lock l(capture->track_lock);
        if (checked) {
            ui->actionTracking->setText("Tracking");
            capture->tracking_mode = Capture::TRACKING_FACE;
        }
        else {
            ui->actionTracking->setText("Untracked");
            capture->tracking_mode = Capture::NOT_TRACKING;
        }
        });
    connect(converter, &Converter::set_roi, this, [this](cv::Rect rect) {
        std::unique_lock l(capture->track_lock);
        if (rect.area() == 0) {
            if (capture->tracking_mode == Capture::STATIC_ROI) {
                ui->actionTracking->setText("Untracked");
                capture->tracking_mode = Capture::NOT_TRACKING;
                ui->actionTracking->setChecked(false);
            }
        }
        else {
            if (capture->tracking_mode == Capture::TRACKING_FACE) {
                capture->tracking_mode = Capture::STATIC_ROI;
                ui->actionTracking->setText("ROI");
            }
            else if (capture->tracking_mode == Capture::NOT_TRACKING) {
                capture->tracking_mode = Capture::STATIC_ROI;
                ui->actionTracking->setText("ROI");
                ui->actionTracking->setChecked(true);
            }
            capture->roi = rect;
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
        ui->label_rotation->setText(QString::number(val) + "\xc2\xb0");
        });


    connect(ui->window_slider, &QSlider::valueChanged, this, [this](int val) {
        show_window_length = (double)val / 10;
        ui->label_window_len->setText(QString::number(show_window_length, 'f', 1)+"s");
        signalProcess->win_length = show_window_length;
        });

    connect(ui->sliderCamOfs, &QSlider::valueChanged, this, [this](int val) {
        signalProcess->cam_ofs = (double)val / 10000;
        ui->labelCamOfs->setText(QString::number((float)val/10, 'f', 1)+"ms");
        });

    
    connect(ui->comboBox_codec, &QComboBox::currentIndexChanged, this, &MainWindow::comb_comp_changed);

    connect(ui->pushButton, &QPushButton::clicked, this, [this]() {
        ui->sliderCamOfs->setValue(0);
        signalProcess->cam_ofs = 0;
        ui->labelCamOfs->setText("0.0ms");
        });
    connect(ui->button_rotation, &QPushButton::clicked, this, [this]() {
        ui->scroll_rotation->setValue(0);
        });
    connect(ui->checkBox_fliplr, &QCheckBox::toggled, this, [this](bool checked) {
        capture->is_fliplr = checked;
        });
    connect(ui->checkBox_flipud, &QCheckBox::toggled, this, [this](bool checked) {
        capture->is_flipud = checked;
        });
    
    ui->VideoBox->setDisabled(true);
    if(!QDir("./rec").exists())
        QDir().mkdir("./rec");
    //connect(capture->signalProcess, &SignalProcess::signalReady, this, &MainWindow::addSignal);

    //connect(capture, &Capture::finished, this, &MainWindow::on_stopButton_clicked);
    //connect(capture, &Capture::finished, this, &MainWindow::capFinished);

    refresh_plot_timer->start(50);
    //connect(capture, &Capture::cap_started, this, [this]() {
    //    setCursor(Qt::ArrowCursor);
    //    });
    capture->start();

    sharedFilePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/phyrecorder_ipc.temp";

    watcher.addPath(sharedFilePath);
    QObject::connect(&watcher, &QFileSystemWatcher::fileChanged,this, &MainWindow::onFileChanged);

    splash.showMessage("Init capture");
    splash.showMessage("Done");
}
static inline int encode_codec_map(PIX_TYPE format) {
    switch (format) {
    case PIX_TYPE_MJPG:return 0;
    case PIX_TYPE_HFYU:return 1;
    case PIX_TYPE_RAW:return 2;
    }
    return -1;
}
void MainWindow::loadCameraOptions(capture::CameraDevice *device) {

    ui->lineEdit_cameraName->disconnect(this);
    ui->lineEdit_cameraName->setText(device->device_friendly_name.c_str());
    connect(ui->lineEdit_cameraName, &QLineEdit::editingFinished, this, [this, device]() {
        auto text = ui->lineEdit_cameraName->text().replace(QRegularExpression("[/\\\\:*?\"'<>|]"), "-");
        device->device_friendly_name = text.toStdString();
        ui->lineEdit_cameraName->setText(text);
        });
    ui->comboBox_codec->setCurrentIndex(encode_codec_map(device->encoder_method));
    ui->slider_quality->setValue(device->encoder_quality);

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
                    camopt_checkBox[opt]->setChecked(true);});
            }
            connect(camopt_pushButton[opt], &QPushButton::clicked, this, [this, opt,device]() {
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

}

void MainWindow::set_sensor_property() {
    for (int opt = 0; opt < capture::CameraDevice::DEVICE_OPTION_CNT; opt++) {
        if ((cam_option_changed & (1 << opt)) != 0) {
            if (opt == capture::CameraDevice::DEVICE_BACKLIGHT
                || opt == capture::CameraDevice::DEVICE_COLOR_ENABLED) {
                capture->selected_device->set_option((capture::CameraDevice::DEVICE_OPTION)opt,
                    { camopt_checkBox[opt]->isChecked() ? 1 : 0,capture::OPTION_MANUAL });
                continue;
            }
            if (camopt_checkBox[opt]->isChecked()) {
                capture->selected_device->set_option((capture::CameraDevice::DEVICE_OPTION)opt,
                    { camopt_slider[opt]->value(),capture::OPTION_MANUAL });
            }
            else {
                capture->selected_device->set_option((capture::CameraDevice::DEVICE_OPTION)opt,
                    { 0,capture::OPTION_AUTO });
            }
        }
    }
    cam_option_changed = 0;
}

void MainWindow::comb_comp_changed(int index) {
    disconnect(comp_conn);
    qDebug() << ui->comboBox_codec->currentText();
    if (ui->comboBox_codec->currentText() == "MJPG") {
        if(capture->selected_device!=nullptr)capture->selected_device->encoder_method = PIX_TYPE_MJPG;
        ui->slider_quality->setMaximum(100);
        ui->slider_quality->setMinimum(0);
        ui->compress_label->setText("Quality");
        comp_conn = connect(ui->slider_quality, &QSlider::valueChanged, this, [this](int val) {
            if (capture->selected_device != nullptr)capture->selected_device->encoder_quality = val;
            if (val == 100)ui->label_quality->setText("MAX");
            else
                ui->label_quality->setText(QString::number(val)+"%");
            });
        ui->slider_quality->setValue(90);
    }
    else if (ui->comboBox_codec->currentText() == "MPNG") {
        if (capture->selected_device != nullptr)capture->selected_device->encoder_method = PIX_TYPE_MPNG;
        ui->slider_quality->setMaximum(9);
        ui->slider_quality->setMinimum(0);
        ui->compress_label->setText("Compression");
        comp_conn = connect(ui->slider_quality, &QSlider::valueChanged, this, [this](int val) {
            ui->label_quality->setText(QString::number(val));
            if (capture->selected_device != nullptr)capture->selected_device->encoder_quality = val;
            });
        ui->slider_quality->setValue(5);
    }
    else if (ui->comboBox_codec->currentText() == "HFYU") {
        if (capture->selected_device != nullptr)capture->selected_device->encoder_method = PIX_TYPE_HFYU;
        ui->slider_quality->setMaximum(0);
        ui->slider_quality->setMinimum(0);
        ui->slider_quality->setValue(0);
        ui->compress_label->setText("Huffman");
        ui->label_quality->setText("NA");
    }
    else if (ui->comboBox_codec->currentText() == "RAW ") {
        if (capture->selected_device != nullptr)capture->selected_device->encoder_method = PIX_TYPE_RAW;
        ui->slider_quality->setMaximum(0);
        ui->slider_quality->setMinimum(0);
        ui->slider_quality->setValue(0);
        ui->compress_label->setText("RawRGB");
        ui->label_quality->setText(QString::number(0));
    }
}
void MainWindow::select_serial(int idx) {
    custom_serial->stop_reading();
    if (!custom_serial->setPort(serial_devices[idx])) {
        comboBox_serial->setCurrentIndex(-1);
    }
}

MainWindow::~MainWindow()
{
    ppg->stop_reading();
    respi->stop_reading();
    delete ui;
}

void MainWindow::refreshCameras() {
    capture::refresh_devices();
    for (auto& [device_name,device] : capture::devices_map) {
        comboBox_cameras->addItem(QString::fromStdString(device_name), QVariant::fromValue(device));
    }
    comboBox_cameras->setCurrentIndex(-1);
}

void MainWindow::on_actionrefreshCamera_triggered() {
    setCursor(Qt::WaitCursor);
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
    run_with_call_back(
        [this]() {
            refreshCameras();
        },
        [this]() {
            comboBox_cameras->setCurrentIndex(-1);
            comboBox_cameras->setDisabled(false);
            ui->actionrefreshCamera->setDisabled(false);
            setCursor(Qt::ArrowCursor);
        }
    );

}

void MainWindow::on_actionstopSerial_triggered() {
    setCursor(Qt::WaitCursor);
    ui->actionstopSerial->setDisabled(true);
    static QThread* th = NULL;
    if (th != NULL) {
        th->wait();
        th->deleteLater();
        th = NULL;
    }
    th = QThread::create([this]() {
        ppg->stop_reading();
        respi->stop_reading();
        custom_serial->stop_reading();
        });
    connect(th, &QThread::finished, this, [this]() {
        ui->actionstopSerial->setDisabled(false);
        comboBox_serial->setCurrentIndex(-1);
        setCursor(Qt::ArrowCursor);
        });
    th->start();
}
void MainWindow::freshSerialDevices() {
    serial_devices.clear();
    comboBox_serial->clear();
    comboBox_serial->setCurrentIndex(-1);
    std::vector<serial::PortInfo>devices_found = serial::list_ports();
    for (const auto& dev : devices_found) {

        if (dev.description.find("Silicon Labs CP210x USB to UART Bridge") == std::string::npos) {
            comboBox_serial->addItem(QString::fromStdString(dev.description));
            serial_devices.push_back(dev);
        }else if (!ppg->isRunning() && ppg->setPort(dev) ||
            !respi->isRunning() && respi->setPort(dev)) {
            continue;
        }
        //else if (ppg->isRunning() && respi->isRunning()) {
        //    break;
        //}
    }
}
void MainWindow::on_actionrefreshSerial_triggered() {
    setCursor(Qt::WaitCursor);
    ui->actionrefreshSerial->setDisabled(true);
    static QThread* th = NULL;
    if (th != NULL) {
        th->wait();
        th->deleteLater();
        th = NULL;
    }
    th = QThread::create([this]() {
        freshSerialDevices();
        });
    connect(th, &QThread::finished, this, [this]() {
        ui->actionrefreshSerial->setDisabled(false);
        comboBox_serial->setCurrentIndex(-1);
        setCursor(Qt::ArrowCursor);
        });
    th->start();
}


void MainWindow::setfft(const QImage& image) {
    ui->fftLabel->setPixmap(QPixmap::fromImage(image));
}

std::string MainWindow::start_record(std::string save_prefix ="") {
    setCursor(Qt::WaitCursor);
    ui->toolBarRC->setDisabled(true);
    ui->actionRecord->setIcon(style()->standardIcon(QStyle::SP_DialogNoButton));
    ui->actionRecord->setToolTip("Stop Recording");
    spinRecordTime->setDisabled(true);
    ui->boxCamOfs->setDisabled(true);
    filenameLineEdit->setDisabled(true);
    ui->VideoBox->setDisabled(true);
    int time = spinRecordTime->value();
    if (time == 0) {
        time = 30;
        spinRecordTime->setValue(30);
    }
    std::string temp_ret;
    if (save_prefix.empty()) {
        auto ts = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        std::stringstream ss;
        if (filenameLineEdit->text().length() != 0)
            ss << ts << "_" << filenameLineEdit->text().toStdString() ;
        else
            ss << ts;
        save_prefix = std::string("./rec/") + ss.str();
        temp_ret = save_prefix;
        save_prefix += "/";
        QDir().mkdir(save_prefix.c_str());
    }
    else {
        if (filenameLineEdit->text().length() != 0) {
            auto save_fname = filenameLineEdit->text().toStdString();
            int first_index = save_prefix.find_first_of('_');
            if (first_index != std::string::npos) {
                auto orig_path = save_prefix.substr(0,first_index+1);
                auto orig_save_fname = save_prefix.substr(first_index +1);
                if (orig_save_fname != save_fname) {
                    save_prefix = orig_path +save_fname+"/";
                    QDir().mkdir(save_prefix.c_str());
                }
                else {
                    save_prefix += (QString("/") + QString::number(QCoreApplication::applicationPid()) + "_").toStdString();
                }
            }
            else {
                save_prefix = save_prefix+"_"+ save_fname + "/";
                QDir().mkdir(save_prefix.c_str());
            }
        }
        else {
            save_prefix += (QString("/") + QString::number(QCoreApplication::applicationPid()) + "_").toStdString();
        }
    }
    std::ranges::replace(save_prefix, ':', '-');
    record_prefix = save_prefix;
    run_with_call_back(
        [this ]() {
            for (int i = 0; i < comboBox_cameras->count(); i++) {
                capture::CameraDevice* device = comboBox_cameras->itemData(i).value<capture::CameraDevice*>();
                if (device->is_running()) {
                    for (auto stream : device->enabled_streams) {
                        auto f_name = record_prefix + device->device_friendly_name + "_" + stream->stream_name + ".avi";
                        std::ranges::replace(f_name, ':', '-');
                        auto v_rec = new MediaWriter(f_name,
                            stream->selected_profile->resolution, stream->selected_profile->ratio, stream->selected_profile->format,
                            device->encoder_method,device->encoder_quality);
                        auto ts_rec = new std::vector<double>;
                        rec_maps[stream] = { v_rec,ts_rec };
                    }
                }
            }
        },
        [this]() {
            is_recording = true;
            record_timer->start();
            ui->toolBarRC->setDisabled(false);
            setCursor(Qt::ArrowCursor);
        }
    );
    return temp_ret;
}
void MainWindow::stop_record() {
    if (record_timer->isActive())
        record_timer->stop();
    setCursor(Qt::WaitCursor);
    ui->toolBarRC->setDisabled(true);
    spinRecordTime->setValue(spin_record_last_time);


    run_with_call_back([this]() {
        recorder_lock.lock();
        is_recording = false;
        recorder_lock.unlock();

        cnpy::npy_save<uint16_t>(record_prefix + "ppg_sig.npy", ppg_sig_rec);
        ppg_sig_rec.clear();
        cnpy::npy_save<double>(record_prefix + "ppg_ts.npy", ppg_ts_rec);
        ppg_ts_rec.clear();
        cnpy::npy_save<uchar>(record_prefix + "respi_sig.npy", respi_sig_rec);
        respi_sig_rec.clear();
        cnpy::npy_save<double>(record_prefix + "respi_ts.npy", respi_ts_rec);
        respi_ts_rec.clear();
        cnpy::npy_save<uint32_t>(record_prefix + "serial_sig.npy", serial_sig_rec);
        serial_sig_rec.clear();
        cnpy::npy_save<double>(record_prefix + "serial_ts.npy", serial_ts_rec);
        serial_ts_rec.clear();

        for (auto& [stream, rec]: rec_maps) {
            rec.first->close();
            auto f_name = record_prefix + stream->device->device_friendly_name + "_" + stream->stream_name + "_ts.npy";
            std::ranges::replace(f_name, ':', '-');
            cnpy::npy_save<double>(f_name, *(rec.second));
            delete rec.first;
            delete rec.second;
        }
        rec_maps.clear();
        },
        [this]() {
            ui->actionRecord->setIcon(style()->standardIcon(QStyle::SP_DialogYesButton));
            ui->actionRecord->setToolTip("Start Recording");
            ui->toolBarRC->setDisabled(false);
            ui->VideoBox->setDisabled(false);
            ui->boxCamOfs->setDisabled(false);
            spinRecordTime->setDisabled(false);
            filenameLineEdit->setDisabled(false);
            setCursor(Qt::ArrowCursor);
        });
    MessageBeep(MB_OK);
    
}
void MainWindow::on_actionRecord_triggered() {
    if (is_recording) {
        emitFileSignal(false);
        stop_record();
    }
    else {
        spin_record_last_time = spinRecordTime->value();
        auto save_path = start_record();
        emitFileSignal(true,save_path.c_str());
        
    }
}

void MainWindow::on_actionStartTrigger_triggered() {
    setCursor(Qt::WaitCursor);
    ui->actionStartTrigger->setDisabled(true);
    capture::CameraDevice* device = comboBox_cameras->currentData().value<capture::CameraDevice*>();
    if (device->is_running()) {
        run_with_call_back([device]() {
                disable_device(device);
            },
            [this, device]() {
                lock_camera_info_play(false);
                ui->actionStartTrigger->setDisabled(false);
                setCursor(Qt::ArrowCursor);
            });
    }
    else {
        static bool device_start_successed;
        run_with_call_back([device]() {
            device_start_successed =enable_device(device);
            },
            [this, device]() {
                if (device_start_successed) {
                    lock_camera_info_play(true);
                }
                ui->actionStartTrigger->setDisabled(false);
                    setCursor(Qt::ArrowCursor);
            });
    }
}


void MainWindow::onFileChanged(const QString& path) {
    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString content = in.readAll();
        file.close();
        QStringList messageParts = content.split(":");

        QString senderPid = messageParts[0];
        QString status = messageParts[1];
        QString currentPid = QString::number(QCoreApplication::applicationPid());
        if (senderPid != currentPid) {
            if (status == "__stop" && is_recording)
                stop_record();
            else if (status == "__start" && !is_recording) {
                spin_record_last_time = spinRecordTime->value();
                spinRecordTime->setValue(-1);
                start_record(messageParts[2].toStdString());
            }
        }

    }
}

bool MainWindow::emitFileSignal(bool is_start,QString msg) {
    QFile file(sharedFilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        QString currentPid = QString::number(QCoreApplication::applicationPid());
        out << currentPid;
        if (is_start)out << ":__start:";
        else out << ":__stop:";
        out << msg;
        file.close();
        return true;
    }
    return false;
}