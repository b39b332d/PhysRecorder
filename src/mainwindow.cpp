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


#include "custom_serial.h"
#include "ppg.h"
#include "respi.h"
#include "cnpy.h"
double win_length=8;

void MainWindow::lock_camera_info_play(bool lock) {
    comboBox_profile_type->setDisabled(lock);
    comboBox_profile->setDisabled(lock);
    comboBox_stream->setDisabled(lock);
    if (lock) {
        ui->VideoBox->setDisabled(false);
        ui->actionStartTrigger->setToolTip("Stop");
        ui->actionStartTrigger->setIcon(style()->standardIcon(QStyle::SP_MediaStop));

        ui->VideoBox->show();
        textedit_streamname->disconnect(this);
        textedit_streamname->setDisabled(false);
        textedit_streamname->setText(capture->selected_device->device_friendly_name.c_str());
        connect(textedit_streamname, &QLineEdit::editingFinished, this, [this]() {
            auto text = textedit_streamname->text().replace(QRegularExpression("[/\\\\:*?\"'<>|]"), "-");
            capture->selected_device->device_friendly_name = text.toStdString();
            textedit_streamname->setText(text);
            });
    }
    else {
        ui->VideoBox->setDisabled(true);
        ui->actionStartTrigger->setToolTip("Play");
        ui->actionStartTrigger->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));

        ui->VideoBox->hide();
        textedit_streamname->disconnect(this);
        textedit_streamname->setDisabled(true);
        textedit_streamname->setText("");
    }
}
static std::atomic<bool> process_thread_running=false;
void Worker::run_in_thread(const std::function<void()>& task) {

    task();
    process_thread_running = false;
    process_thread_running.notify_one();
    emit process_finished();
}
void MainWindow::run_with_call_back(const std::function<void()>& run_in_thread, const std::function<void()>& call_back = []() {}) {
    bool d = false;
    while (!process_thread_running.compare_exchange_strong(d, true)) {
        process_thread_running.wait(true);
    }
    connect(&process_thread_worker,&Worker::process_finished, 
        this,call_back,Qt::SingleShotConnection);
    QMetaObject::invokeMethod(&process_thread_worker,
        &Worker::run_in_thread,Qt::QueuedConnection,run_in_thread);

}

void MainWindow::refresh_plot() {

    double ts = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count() + 0.1;
    ui->plot_interp->xAxis->setRange(ts + signalProcess->cam_ofs, show_window_length, Qt::AlignRight);
    ui->plot_interp->xAxis2->setRange(ts, show_window_length, Qt::AlignRight);
    ui->plot_interp->yAxis->rescale();
    ui->plot_interp->yAxis2->rescale();
    ui->plot_interp->replot();

    ui->plot_ppg->xAxis->setRange(ts, show_window_length, Qt::AlignRight);
    ui->plot_ppg->yAxis2->rescale();
    ui->plot_ppg->replot();
}

void MainWindow::onDeviceSelected(capture::CameraDevice* device) {
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

void MainWindow::onCaptureDeviceDisabled(capture::CameraDevice* device) {
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
        return;
    }
    setCursor(Qt::WaitCursor);
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
        //onStreamHighted(idx, true);
        stream->device->register_stream(stream->selected_profile);
        if(comboBox_stream->getSelectedItems().size()!=0)
            ui->actionStartTrigger->setDisabled(false);
        else
            ui->actionStartTrigger->setDisabled(true);
    }
    else {
        //onStreamHighted(idx, false);
        stream->device->unregister_stream(stream);
    }
}
void MainWindow::onStreamHighted(int idx, bool is_highlight) {

    if (is_highlight) {
        capture::CameraStream* stream = comboBox_stream->itemData(idx).value<capture::CameraStream*>();
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
        comboBox_profile_type->clear();
        comboBox_profile_type->setCurrentIndex(-1);
        comboBox_profile->clear();
        comboBox_profile->setCurrentIndex(-1);
    }
}

void MainWindow::onSerialHighted(int idx, bool is_highlight) {
    //if (idx == -1) return; // deselect all
    SerialReader* stream = comboBox_serial->itemData(idx).value<SerialReader*>();
    auto graph = stream->graph;
    if (is_highlight) {
        if (graph) {
            graph->setValueAxis(ui->plot_ppg->yAxis2);
            ui->plot_ppg->legend->itemWithPlottable(graph)->setSelected(true);
            graph->setSelection(QCPDataSelection(QCPDataRange(0,500)));
            signalProcess->graph_ppg->setSelection(QCPDataSelection(QCPDataRange(0, 500)));
            connect(stream, &SerialReader::serial_ready, signalProcess, &SignalProcess::processPPG);

            textedit_serialname->disconnect(this);
            textedit_serialname->setText(QString::fromStdString(stream->friendly_name));
            textedit_serialname->setDisabled(false);
            connect(textedit_serialname, &QLineEdit::editingFinished, this,
                [stream, graph,this]() {
                    auto text = textedit_serialname->text().replace(QRegularExpression("[/\\\\:*?\"'<>|]"), "-");
                    stream->friendly_name = text.toStdString();
                    textedit_serialname->setText(text);
                    graph->setName(text);
                });
        }
    }
    else {
        if (graph) {
            QMetaObject::invokeMethod(signalProcess, &SignalProcess::reset_ppg);
            disconnect(stream, &SerialReader::serial_ready, signalProcess, &SignalProcess::processPPG);
            graph->setValueAxis(ui->plot_ppg->yAxis);
            graph->setSelection(QCPDataSelection());
            signalProcess->graph_ppg->setSelection(QCPDataSelection(QCPDataRange()));
            ui->plot_ppg->legend->itemWithPlottable(graph)->setSelected(false);
            textedit_serialname->disconnect(this);
            textedit_serialname->setText("");
            textedit_serialname->setDisabled(true);
        }
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
{
    splash.showMessage("Setting up UI");
    ui->setupUi(this);

    process_thread_worker.moveToThread(&process_thread);
    process_thread.start();

    this->setWindowTitle("Remote PhotoPlethysmoGraphy");

    comboBox_cameras = new QComboBox(ui->toolBarRS);
    comboBox_stream = new MultiSelectComboBox(ui->toolBarRS);
    comboBox_profile_type = new QComboBox(ui->toolBarRS);
    comboBox_profile = new QComboBox(ui->toolBarRS);
    comboBox_cameras->setFixedWidth(225);
    comboBox_stream->setFixedWidth(110);
    comboBox_profile_type->setFixedWidth(60);
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
    ui->actionRecord->setIconSize(QSize(30, 30));
    ui->actionRecord->setIcon(style()->standardIcon(QStyle::SP_DialogYesButton));
    connect(ui->actionRecord, &QPushButton::clicked, this, &MainWindow::onRecordToggled);

    ui->actionrefreshCamera->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    ui->actionrefreshSerial->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    ui->actionstopSerial->setIcon(style()->standardIcon(QStyle::SP_MediaStop));


    comboBox_serial = new MultiSelectComboBox(ui->toolBarSD);
    comboBox_serial->setFixedWidth(150);
    ui->toolBarSD->insertWidget(ui->actionstopSerial, comboBox_serial);
    connect(comboBox_serial, &MultiSelectComboBox::selectionChanged, this, &MainWindow::onSerialSelected);
    connect(comboBox_serial, &MultiSelectComboBox::heightSelect, this, &MainWindow::onSerialHighted);


    textedit_serialname = new QLineEdit(ui->toolBarSD);
    textedit_serialname->setMinimumWidth(150);
    textedit_serialname->setMaximumWidth(200);
    textedit_serialname->setDisabled(true);
    textedit_serialname->setPlaceholderText("Serial Filename");
    ui->toolBarSD->addWidget(textedit_serialname);

    textedit_streamname = new QLineEdit(ui->toolBarRS);
    textedit_streamname->setMaximumWidth(200);
    textedit_streamname->setDisabled(true);
    textedit_streamname->setPlaceholderText("Device Filename");
    ui->toolBarRS->addWidget(textedit_streamname);


    record_timer = new QTimer(this);
    record_timer->setSingleShot(false);
    record_timer->setInterval(1000);
    connect(record_timer, &QTimer::timeout, this, [this]() {
        if (ui->spinRecordTime->value() != -1) {
            ui->spinRecordTime->setValue(ui->spinRecordTime->value() - 1);
            if (ui->spinRecordTime->value() == 0) {
                emitFileSignal(0);
                stop_record();
            }
        }
        });
    splash.showMessage("Detecting Realsense Device...");
    on_actionrefreshCamera_triggered();

    splash.showMessage("Init Plot...");

    refresh_plot_timer = new QTimer(this);
    connect(refresh_plot_timer, &QTimer::timeout, this, &MainWindow::refresh_plot);
    QSharedPointer<QCPAxisTickerDateTime> timeTicker(new QCPAxisTickerDateTime);
    timeTicker->setDateTimeFormat("mm:ss");
    ui->plot_ppg->legend->setVisible(true);
    ui->plot_ppg->legend->setBrush(QBrush(QColor(255, 255, 255, 230)));

    ui->plot_ppg->xAxis->setTicker(timeTicker);
    ui->plot_ppg->yAxis->setRange(-0.05, 1.05);


    QFont legendFont = font();
    legendFont.setPointSize(10);
    ui->plot_ppg->setInteractions( QCP::iSelectLegend | QCP::iSelectPlottables);
    ui->plot_ppg->legend->setFont(legendFont);
    ui->plot_ppg->legend->setSelectedFont(legendFont);
    ui->plot_ppg->legend->setSelectableParts(QCPLegend::spItems);
    connect(ui->plot_ppg, &QCustomPlot::selectionChangedByUser, this, &MainWindow::onSerialGraphSelectionChanged);


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
    connect(capture, &Capture::device_disabled, this, &MainWindow::onCaptureDeviceDisabled);
    connect(capture->signalProcess, &SignalProcess::fftReady, this, &MainWindow::setfft);
    auto o = QObject::connect(converter, &Converter::frameReady, ui->q_video, &ImageViewer::setImage);
    connect(converter, &Converter::device_selected, this, &MainWindow::onDeviceSelected);

    converterThread->start();

    splash.showMessage("Detecting Serial Device...");
    

    on_actionrefreshSerial_triggered();

    ui->VideoBox->hide();


    splash.showMessage("Connect signals");

    ui->actionR->setProperty("channel",  signalProcess->r_channel);
    ui->actionG->setProperty("channel", signalProcess->g_channel);
    ui->actionB->setProperty("channel", signalProcess->b_channel);
    ui->actionPOS->setProperty("channel", signalProcess->pos_channel);
    connect(ui->actionR, &QPushButton::toggled, capture->signalProcess, &SignalProcess::setShowChannel);
    connect(ui->actionG, &QPushButton::toggled, capture->signalProcess, &SignalProcess::setShowChannel);
    connect(ui->actionB, &QPushButton::toggled, capture->signalProcess, &SignalProcess::setShowChannel);
    connect(ui->actionPOS, &QPushButton::toggled, capture->signalProcess, &SignalProcess::setShowChannel);
    connect(ui->actionR, &QPushButton::toggled, [this](bool checked) {if(!checked) ui->actionR->setText("R"); });
    connect(ui->actionG, &QPushButton::toggled, [this](bool checked) {if (!checked)ui->actionG->setText("G"); });
    connect(ui->actionB, &QPushButton::toggled, [this](bool checked) {if (!checked)ui->actionB->setText("B"); });
    connect(ui->actionPOS, &QPushButton::toggled, [this](bool checked) {if (!checked)ui->actionPOS->setText("POS"); });

    connect(capture->signalProcess, &SignalProcess::sqiReady, this, &MainWindow::setSqi);

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
        win_length = show_window_length;
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
    capture->start();

    sharedFilePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/phyrecorder_ipc.temp";

    watcher.addPath(sharedFilePath);
    QObject::connect(&watcher, &QFileSystemWatcher::fileChanged,this, &MainWindow::onFileChanged);

    splash.showMessage("Init capture");

    process_thread_running.wait(true);
    splash.showMessage("Done");
}
void MainWindow::setSqi(int c, float snr, float sqi) {
    QString v = QString::number(snr, 'f', 2) + "\n" + QString::number(sqi, 'f', 2);
    if (c == signalProcess->r_channel) {
        if (ui->actionR->isChecked()) {
            ui->actionR->setText(v);
        }
        else {
            ui->actionR->setText("R");
        }
    }else if (c == signalProcess->g_channel) {
        if (ui->actionG->isChecked()) {
            ui->actionG->setText(v);
        }
        else {
            ui->actionG->setText("G");
        }
    }
    else if(c == signalProcess->b_channel) {
        if (ui->actionB->isChecked()) {
            ui->actionB->setText(v);
        }
        else {
            ui->actionB->setText("B");
        }
    }
    else if(c == signalProcess->pos_channel) {
        if (ui->actionPOS->isChecked()) {
            ui->actionPOS->setText(v);
        }
        else {
            ui->actionPOS->setText("POS");
        }
    }

}
void MainWindow::onSerialGraphSelectionChanged() {
    int h_serial = comboBox_serial->getHightLight();
    QCPGraph* s_graph = nullptr;
    if(h_serial!=-1)
        s_graph = comboBox_serial->itemData(h_serial).value<SerialReader*>()->graph;

    for (int i = 0; i < ui->plot_ppg->graphCount(); ++i)
    {
        QCPGraph* graph = ui->plot_ppg->graph(i);
        QCPPlottableLegendItem* item = ui->plot_ppg->legend->itemWithPlottable(graph);
        bool is_select = (item->selected() || graph->selected());

        if ((s_graph == graph)&& !is_select) {
            comboBox_serial->setHighLight(h_serial, false);
        }
        else if((s_graph != graph) && is_select) {
            for (int c = 0; c < comboBox_serial->count();c++) {
                s_graph = comboBox_serial->itemData(c).value<SerialReader*>()->graph;
                if (s_graph == graph) {
                    comboBox_serial->setHighLight(c, true);
                }
            }
            return;
        }
        else if ((s_graph == graph) && is_select) {
            //select an already selected graph
            comboBox_serial->setHighLight(h_serial, false);
            return;
        }
    }
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
void MainWindow::comb_comp_changed(int index) {
    capture::CameraDevice* device = comboBox_cameras->currentData().value<capture::CameraDevice*>();
    std::vector<std::set<PIX_TYPE>> support_pixs;
    for (auto s : device->enabled_streams) {
        support_pixs.push_back( MediaWriter::get_supported_encoders(s->selected_profile->format));
    }
    auto support_pix =  intersectSets(support_pixs);

    ui->slider_quality->disconnect(this);
    qDebug() << ui->comboBox_codec->currentText();
    if (ui->comboBox_codec->currentText() == "MJPG") {
        if (support_pix.find(PIX_TYPE_MJPG) == support_pix.end()) {
            auto new_t = *(support_pix.begin());
            ui->comboBox_codec->setCurrentIndex(
                ui->comboBox_codec->findText(QString::fromStdString(
                    GET_PIX_TYPE_NAME(new_t))));
            return;
        }
        if(capture->selected_device!=nullptr)capture->selected_device->encoder_method = PIX_TYPE_MJPG;
        ui->slider_quality->setMaximum(100);
        ui->slider_quality->setMinimum(0);
        ui->compress_label->setText("Quality");
        connect(ui->slider_quality, &QSlider::valueChanged, this, [this](int val) {
            if (capture->selected_device != nullptr)capture->selected_device->encoder_quality = val;
            if (val == 100)ui->label_quality->setText("MAX");
            else
                ui->label_quality->setText(QString::number(val)+"%");
            });
        ui->slider_quality->setValue(90);
    }
    else if (ui->comboBox_codec->currentText() == "HFYU") {
        if (support_pix.find(PIX_TYPE_HFYU) == support_pix.end()) {
            auto new_t = *(support_pix.begin());
            ui->comboBox_codec->setCurrentIndex(
                ui->comboBox_codec->findText(QString::fromStdString(
                    GET_PIX_TYPE_NAME(new_t))));
            return;
        }
        if (capture->selected_device != nullptr)capture->selected_device->encoder_method = PIX_TYPE_HFYU;
        ui->slider_quality->setMaximum(0);
        ui->slider_quality->setMinimum(0);
        ui->slider_quality->setValue(0);
        ui->compress_label->setText("Huffman");
        ui->label_quality->setText("NA");
    }
    else if (ui->comboBox_codec->currentText() == "RAW ") {
        if (support_pix.find(PIX_TYPE_RAW) == support_pix.end()) {
            auto new_t = *(support_pix.begin());
            ui->comboBox_codec->setCurrentIndex(
                ui->comboBox_codec->findText(QString::fromStdString(
                    GET_PIX_TYPE_NAME(new_t))));
            return;
        }
        if (capture->selected_device != nullptr)capture->selected_device->encoder_method = PIX_TYPE_RAW;
        ui->slider_quality->setMaximum(0);
        ui->slider_quality->setMinimum(0);
        ui->slider_quality->setValue(0);
        ui->compress_label->setText("RawRGB");
        ui->label_quality->setText(QString::number(0));
    }
}
void MainWindow::onSerialSelected(int idx,bool selected) {
    SerialReader* serial_reader = comboBox_serial->itemData(idx).value<SerialReader*>();
    setCursor(Qt::WaitCursor);
    static bool is_stop_successed = false;
    if (selected) {
        auto graph = ui->plot_ppg->addGraph();
        graph->setValueAxis(ui->plot_ppg->yAxis);
        graph->setPen(QPen(QColor(255, 110, 40)));
        graph->setName(QString::fromStdString(serial_reader->friendly_name));
        serial_reader->graph = graph;
        connect(serial_reader, &SerialReader::serial_stopped,
            this, &MainWindow::onSerialStopped);
        serial_reader->start_reading();
        setCursor(Qt::ArrowCursor);
    }
    else {
        run_with_call_back([this, serial_reader]() {
            serial_reader->disconnect(this);
            is_stop_successed = serial_reader->stop_reading();
            }, [this, serial_reader]() {
                if(is_stop_successed)
                    onSerialStopped(serial_reader);
                setCursor(Qt::ArrowCursor);
                });
        
    }
}
void MainWindow::onSerialStopped(SerialReader* reader) {

    if (reader->graph) {
        comboBox_serial->selectItem(QVariant::fromValue(reader), false);
        reader->graph->removeFromLegend();
        ui->plot_ppg->removeGraph(reader->graph);
        reader->graph = nullptr;
    }
    if (reader->is_invalid) {
        reader->disconnect(this);
        auto it = rec_SerialReaders.find(reader);
        if (it != rec_SerialReaders.end()) {
            rec_SerialReaders.erase(it);
            reader->stopRecording();
        }
        comboBox_serial->removeItem(QVariant::fromValue(reader));
    }
}

MainWindow::~MainWindow()
{
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

    auto high_idx = comboBox_serial->getHightLight();
    if (high_idx == -1) {
        static QSet<SerialReader*> successed_stopped;
        run_with_call_back([this]() {
            for (auto s : comboBox_serial->getSelectedItems()) {
                SerialReader* serial_reader = comboBox_serial->itemData(s).value<SerialReader*>();
                serial_reader->disconnect(this);
                if (serial_reader->stop_reading()) {
                    successed_stopped.insert(serial_reader);
                }

            }            
            }, [this]() {
                for (auto s : successed_stopped) {
                    onSerialStopped(s);
                }
                ui->actionstopSerial->setDisabled(false);
                setCursor(Qt::ArrowCursor);
         });
    }
    else {
        onSerialSelected(high_idx, false);
        ui->actionstopSerial->setDisabled(false);
        setCursor(Qt::ArrowCursor);
    }
}
void MainWindow::on_actionrefreshSerial_triggered() {
    setCursor(Qt::WaitCursor);
    ui->actionrefreshSerial->setDisabled(true);
    int c_highidx = comboBox_serial->getHightLight();
    if(c_highidx !=-1)
        onSerialHighted(c_highidx, false);
    comboBox_serial->clear();

    run_with_call_back([this]() {
        SerialReader::refresh_serials();
        }, 
    [this]() {
        for (auto& [dev_name, dev] : SerialReader::serial_readers) {
            comboBox_serial->addItem(QString::fromStdString(dev->device_name),
                QVariant::fromValue(dev),
                dev->is_running);
        }
        ui->actionrefreshSerial->setDisabled(false);
        setCursor(Qt::ArrowCursor);
        });
}


void MainWindow::setfft(const QImage& image) {
    ui->fftLabel->setPixmap(QPixmap::fromImage(image));
}

std::string MainWindow::start_record(std::string save_prefix ="") {
    setCursor(Qt::WaitCursor);
    ui->toolBarRC->setEnabled(false);
    ui->actionRecord->setIcon(style()->standardIcon(QStyle::SP_DialogNoButton));
    ui->actionRecord->setToolTip("Stop Recording");
    ui->spinRecordTime->setDisabled(true);
    ui->boxCamOfs->setDisabled(true);
    ui->filenameLineEdit->setDisabled(true);
    ui->VideoBox->setDisabled(true);
    int time = ui->spinRecordTime->value();
    if (time == 0) {
        time = 30;
        ui->spinRecordTime->setValue(30);
    }
    std::string temp_ret;
    filenameLineEdit_name = ui->filenameLineEdit->text();
    if (save_prefix.empty()) {
        auto ts = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        std::stringstream ss;
        if (filenameLineEdit_name.length() != 0) {
            ss << ts << "_" << filenameLineEdit_name.toStdString();
        }
        else {
            ss << ts;
        }
        save_prefix = std::string("./rec/") + ss.str();
        temp_ret = save_prefix;
        save_prefix += "/";
        QDir().mkdir(save_prefix.c_str());
    }
    else {
        if (filenameLineEdit_name.length() != 0) {
            auto save_fname = filenameLineEdit_name.toStdString();
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
    ui->filenameLineEdit->setText("<"+QString::fromStdString(save_prefix)+">");
    std::ranges::replace(save_prefix, ':', '-');
    run_with_call_back(
        [this, save_prefix]() {
            for (int i = 0; i < comboBox_cameras->count(); i++) {
                capture::CameraDevice* device = comboBox_cameras->itemData(i).value<capture::CameraDevice*>();
                if (device->is_running()) {
                    for (auto stream : device->enabled_streams) {
                        auto f_name = save_prefix + device->device_friendly_name + "_" + stream->stream_name;
                        std::ranges::replace(f_name, ':', '-');
                        auto v_rec = new MediaWriter(f_name,
                            stream->selected_profile->resolution, stream->selected_profile->ratio, stream->selected_profile->format,
                            device->encoder_method,device->encoder_quality);
                        rec_maps[stream] = v_rec;
                    }
                }
            }
            for (auto s : comboBox_serial->getSelectedItems()) {
                SerialReader* reader = comboBox_serial->itemData(s).value<SerialReader*>();
                reader->startRecording(save_prefix + reader->friendly_name);
                rec_SerialReaders.insert(reader);
            }
        },
        [this]() {
            is_recording = true;
            record_timer->start();
            ui->toolBarRC->setEnabled(true);
            setCursor(Qt::ArrowCursor);
        }
    );
    return temp_ret;
}
void MainWindow::stop_record() {
    if (record_timer->isActive())
        record_timer->stop();
    setCursor(Qt::WaitCursor);
    ui->toolBarRC->setEnabled(false);
    ui->spinRecordTime->setValue(spin_record_last_time);
    ui->filenameLineEdit->setText(filenameLineEdit_name);

    run_with_call_back([this]() {
        recorder_lock.lock();
        is_recording = false;
        recorder_lock.unlock();


        for (auto s : rec_SerialReaders)  s->stopRecording();
            rec_SerialReaders.clear();
        for (auto& [stream, rec]: rec_maps) delete rec;
            rec_maps.clear();
        },
        [this]() {
            ui->actionRecord->setIcon(style()->standardIcon(QStyle::SP_DialogYesButton));
            ui->actionRecord->setToolTip("Start Recording");
            ui->toolBarRC->setEnabled(true);
            ui->VideoBox->setDisabled(false);
            ui->boxCamOfs->setDisabled(false);
            ui->spinRecordTime->setDisabled(false);
            ui->filenameLineEdit->setDisabled(false);
            setCursor(Qt::ArrowCursor);
        });
    MessageBeep(MB_OK);
    
}
void MainWindow::onRecordToggled() {
    if (is_recording) {
        emitFileSignal(false);
        stop_record();
    }
    else {
        spin_record_last_time = ui->spinRecordTime->value();
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
                spin_record_last_time = ui->spinRecordTime->value();
                ui->spinRecordTime->setValue(-1);
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