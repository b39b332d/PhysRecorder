#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "imageviewer.h"
#include<qdebug>
#include <QtWidgets>
#include <string>
#include <thread>
#include <QSerialPortInfo>

void MainWindow::refresh_plot() {
    static uchar ref_cnt = 0;
    static bool r_status = true , g_status=true, b_status = true;
    static bool ppgStatusFinished = true;
    static bool respiStatusFinished = true;
    if (ppg->isFinished()) {
        if (!ppgStatusFinished) {
            ppgStatusFinished = true;
            ui->plot_ppg->graph(0)->removeFromLegend();
        }
    }else{
        if (ppgStatusFinished) {
            ppgStatusFinished = false;
            ui->plot_ppg->graph(0)->addToLegend();
        }
    }
    if (respi->isFinished()) {
        if (!respiStatusFinished) {
            respiStatusFinished = true;
            ui->plot_ppg->graph(1)->removeFromLegend();
        }
    }
    else {
        if (respiStatusFinished) {
            respiStatusFinished = false;
            ui->plot_ppg->graph(1)->addToLegend();
        }
    }
    double ts = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count() + 0.1;
    ui->plot_ppg->xAxis->setRange(ts, show_window_length, Qt::AlignRight);
    if (ref_cnt++ % 10 == 0) {
        ui->plot_ppg->graph(1)->data()->removeBefore(ts - show_window_length);
        ui->plot_ppg->graph(0)->data()->removeBefore(ts - show_window_length);
        ui->plot_interp->graph(0)->data()->removeBefore(ts - show_window_length);
        ui->plot_interp->graph(1)->data()->removeBefore(ts - show_window_length);
        ui->plot_interp->graph(2)->data()->removeBefore(ts - show_window_length);
        ui->plot_interp->graph(3)->data()->removeBefore(ts - show_window_length);
        if (ui->actionR->isChecked() != r_status) {
            r_status = !r_status;
            ui->plot_interp->graph(0)->setVisible(r_status);
        }
        if (ui->actionG->isChecked() != g_status) {
            g_status = !g_status;
            ui->plot_interp->graph(1)->setVisible(g_status);
        }
        if (ui->actionB->isChecked() != b_status) {
            b_status = !b_status;
            ui->plot_interp->graph(2)->setVisible(b_status);
        }
    }
    ui->plot_ppg->replot();

    ui->plot_interp->xAxis->setRange(ts + signalProcess->cam_ofs, show_window_length, Qt::AlignRight);
    ui->plot_interp->xAxis2->setRange(ts, show_window_length, Qt::AlignRight);
    ui->plot_interp->yAxis->rescale();
    ui->plot_interp->yAxis2->rescale();
    ui->plot_interp->replot();
}
void MainWindow::set_profile(int idx) {
    ui->toolBarRS->setDisabled(true);
    QThread* thread = QThread::create([this,idx]() {
        if (is_opened)
            sensor.close();
        profile = profiles[idx];
        rs2::video_stream_profile video_stream_profile = profile.as<rs2::video_stream_profile>();
        is_opened = true;
        sensor.open(profile);
        cam_option_changed=0xFFFF;
        set_sensor_property();
        sensor.set_option(RS2_OPTION_FRAMES_QUEUE_SIZE, 1);
        sensor.set_option(RS2_OPTION_AUTO_EXPOSURE_PRIORITY, 0);
        capture->setCapture(sensor, video_stream_profile.width(), video_stream_profile.height(), video_stream_profile.fps());
        });
    connect(thread, &QThread::finished, this, [this]() {
        ui->toolBarRS->setDisabled(false);
        ui->actionStartTrigger->setDisabled(false);
        });
    thread->start();

}
void MainWindow::plotPPG(uint16_t signal,double ts) {
    ui->plot_ppg->graph(0)->addData(ts, (double)signal/1024);
    //if (!refresh_plot_timer->isActive())
    //    refresh_plot_timer->start(20);
    if (isRecording) {
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
    if (isRecording) {
        respi_ts_rec.push_back(ts);
        respi_sig_rec.push_back(signal);
    }
    // rescale value (vertical) axis to fit the current data:
    //ui->plot_ppg->graph(1)->rescaleValueAxis();
}
void MainWindow::saveSignals() {
    ui->toolBarRC->setDisabled(true);
    isRecording = false;
    QThread* thread = QThread::create([this]() {
        std::string fname = "./rec/";
        fname += std::string(capture->save_path);
        cnpy::npy_save(fname+"ppg_sig.npy", ppg_sig_rec);
        ppg_sig_rec.clear();
        cnpy::npy_save(fname + "ppg_ts.npy", ppg_ts_rec);
        ppg_ts_rec.clear();
        cnpy::npy_save(fname + "respi_sig.npy", respi_sig_rec);
        respi_sig_rec.clear();
        cnpy::npy_save(fname + "respi_ts.npy", respi_ts_rec);
        respi_ts_rec.clear();
        capture->wait_for_rec_save();
        });
    connect(thread, &QThread::finished, this, [this]() {
        ui->toolBarRC->setDisabled(false);
        ui->VideoBox->setDisabled(false);
        ui->boxCamOfs->setDisabled(false);
        });
    thread->start();
}

void MainWindow::plotInterpPPG(double ppg, double ts) {
    ui->plot_interp->graph(3)->addData(ts, ppg);
}
void MainWindow::plotInterpRGB(float r, float g, float b, double ts, float* sqi) {
    //qDebug() << r << g << b << ts << QThread::currentThreadId();
    ui->plot_interp->graph(0)->addData(ts, r);
    ui->plot_interp->graph(1)->addData(ts, g);
    ui->plot_interp->graph(2)->addData(ts, b);
    if (sqi != NULL) {
        ui->label_SQIr->setText(QString::number(sqi[0], 'f', 2));
        ui->label_SQIg->setText(QString::number(sqi[1], 'f', 2));
        ui->label_SQIb->setText(QString::number(sqi[2], 'f', 2));
        ui->label_SNRr->setText(QString::number(sqi[3], 'f', 2));
        ui->label_SNRg->setText(QString::number(sqi[4], 'f', 2));
        ui->label_SNRb->setText(QString::number(sqi[5], 'f', 2));
        delete[] sqi;
    }
    //if (!refresh_plot_timer->isActive())
    //    refresh_plot_timer->start(20);
}
void MainWindow::detect_profiles(int idx) {
    comboBox_profile->clear();
    ui->actionStartTrigger->setDisabled(true);
    device = devices[idx];
    std::thread* th = new std::thread([this]() {
        if (is_opened)
            sensor.close();
        is_opened = false;
        std::vector<rs2::sensor> sensors = device.query_sensors();
        for (rs2::sensor sensor : sensors)
        {
            if (sensor.supports(RS2_CAMERA_INFO_NAME) && std::string("RGB Camera") == sensor.get_info(RS2_CAMERA_INFO_NAME)) {
                this->sensor = sensor;
                std::vector<rs2::stream_profile> stream_profiles = sensor.get_stream_profiles();
                int profile_num = 0;
                profiles.clear();
                for (rs2::stream_profile stream_profile : stream_profiles)
                {
                    rs2_stream stream_data_type = stream_profile.stream_type();
                    std::string stream_name = stream_profile.stream_name();
                    rs2::video_stream_profile video_stream_profile = stream_profile.as<rs2::video_stream_profile>();
                    if (stream_name == "Color" && video_stream_profile.format() == rs2_format::RS2_FORMAT_BGR8) {
                        profiles.push_back(stream_profile);
                        comboBox_profile->addItem(QString::fromStdString((std::ostringstream() << video_stream_profile.width() << "x" << video_stream_profile.height() << "@ " << video_stream_profile.fps() << "Hz").str()));
                        comboBox_profile->setCurrentIndex(-1);
                    }
                    profile_num++;
                }
                break;
            }
        }
        }
    );
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
    //connect(ui->videospeed, &QSlider::valueChanged, ui->speed_spin, &QSpinBox::setValue);
    //connect(ui->speed_spin, &QSpinBox::valueChanged, ui->videospeed, &QSlider::setValue);

    comboBox_cameras = new QComboBox(ui->toolBarRS);
    comboBox_profile = new QComboBox(ui->toolBarRS);
    comboBox_cameras->setFixedWidth(225);
    comboBox_profile->setFixedWidth(130);
    ui->toolBarRS->addWidget(comboBox_cameras);
    ui->toolBarRS->addWidget( comboBox_profile);
    connect(comboBox_cameras, &QComboBox::activated, this, &MainWindow::detect_profiles);
    connect(comboBox_profile, &QComboBox::activated, this, &MainWindow::set_profile);
    ui->actionStartTrigger->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    ui->actionRecord->setIcon(style()->standardIcon(QStyle::SP_DialogYesButton));
    ui->actionrefreshRealsense->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    ui->actionrefreshSerial->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    ui->actionstopSerial->setIcon(style()->standardIcon(QStyle::SP_MediaStop));

    spinRecordTime = new QSpinBox(ui->toolBarRC);
    spinRecordTime->setMaximum(9999);
    spinRecordTime->setMinimum(0);
    spinRecordTime->setValue(9999);
    ui->toolBarRC->addWidget(spinRecordTime);
    record_timer = new QTimer(this);
    record_timer->setSingleShot(false);
    record_timer->setInterval(1000);
    connect(record_timer, &QTimer::timeout, this, [this]() {
        spinRecordTime->setValue(spinRecordTime->value()-1);
        if (spinRecordTime->value() == 0) {
            on_actionRecord_triggered();
        }
        });
    splash.showMessage("Detecting Realsense Device...");
    on_actionrefreshRealsense_triggered();



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
    ui->plot_ppg->graph(0)->removeFromLegend();
    ui->plot_ppg->graph(1)->removeFromLegend();


/*    ui->plot_interp->legend->setVisible(true);
    ui->plot_interp->legend->setBrush(QBrush(QColor(255, 255, 255, 230)))*/;
    ui->plot_interp->xAxis2->setTicker(timeTicker);
    ui->plot_interp->addGraph();
    ui->plot_interp->graph(0)->setPen(QPen(QColorConstants::Red));
    ui->plot_interp->addGraph();
    ui->plot_interp->graph(1)->setPen(QPen(QColorConstants::Green));
    ui->plot_interp->addGraph();
    ui->plot_interp->graph(2)->setPen(QPen(QColorConstants::Blue));
    ui->plot_interp->addGraph(ui->plot_interp->xAxis2, ui->plot_interp->yAxis2);
    ui->plot_interp->graph(3)->setPen(QPen(QColorConstants::Black));
    //ui->plot_interp->yAxis2->setVisible(true);
    ui->plot_interp->xAxis2->setVisible(true);
    ui->plot_interp->xAxis->setVisible(false);
    ui->AFcomboBox->addItem("None");
    ui->AFcomboBox->addItem("50Hz");
    ui->AFcomboBox->addItem("60Hz");
    ui->AFcomboBox->addItem("Auto");
    //on_actionrefreshSerial_triggered();

    signalProcess = new SignalProcess(ui->fftLabel);
    auto th2 = new QThread();
    signalProcess->moveToThread(th2);
    th2->start();

    splash.showMessage("Creating threads");
    converter = new Converter(ui->q_video);
    QThread* converterThread = new QThread();

    capture = new Capture(*converter, signalProcess);
    converter->moveToThread(converterThread);
    connect(capture->signalProcess, &SignalProcess::fftReady, this, &MainWindow::setfft);
    connect(converter, &Converter::frameReady, ui->q_video, &ImageViewer::setImage);
    //connect(converter, &Converter::setColor, this, &MainWindow::setColorReady);
    connect(converterThread, &QThread::started, converter, &Converter::start);
    converterThread->start();

    splash.showMessage("Detecting Serial Device...");

    respi = new RESPIReader();
    connect(respi, &RESPIReader::respiReady, this, &MainWindow::plotRESPI);

    ppg = new PPGReader();
    connect(ppg, &PPGReader::ppgReady, this, &MainWindow::plotPPG);
    connect(ppg, &PPGReader::ppgReady, capture->signalProcess, &SignalProcess::processPPG);

    freshSerialDevices();




    splash.showMessage("Connect signals");
    connect(ui->actionR, &QPushButton::toggled,capture->signalProcess, &SignalProcess::setShowR);
    connect(ui->actionG, &QPushButton::toggled, capture->signalProcess, &SignalProcess::setShowG);
    connect(ui->actionB, &QPushButton::toggled, capture->signalProcess, &SignalProcess::setShowB);

    connect(capture->signalProcess, &SignalProcess::interpPPGReady, this, &MainWindow::plotInterpPPG);
    connect(capture->signalProcess, &SignalProcess::interpRGBReady, this, &MainWindow::plotInterpRGB);
    connect(capture, &Capture::signalReady, capture->signalProcess, &SignalProcess::processSignal);
    connect(ui->actionTracking, &QPushButton::toggled, this, [this](bool checked) {
        capture->tracking = checked;
        });
    connect(ui->slider_exposure, &QSlider::valueChanged, this, [this](int val) {
        ui->label->setText(QString::number(val));
        cam_option_changed |= 0x0004;
        if (!cam_option_changed_timer.isActive())cam_option_changed_timer.start(100);
        });
    connect(ui->slider_white, &QSlider::valueChanged, this, [this](int val) {
        ui->label_6->setText(QString::number(val * 10)+"k");
        cam_option_changed |= 0x0020;
        if (!cam_option_changed_timer.isActive())cam_option_changed_timer.start(100);
        });
    connect(ui->gainSlider, &QSlider::valueChanged, this, [this](int val) {
        ui->label_4->setText(QString::number(val));
        cam_option_changed |= 0x0008;
        if (!cam_option_changed_timer.isActive())cam_option_changed_timer.start(100);
        });
    connect(ui->scroll_rot, &QScrollBar::valueChanged, this, [this](int val) {
        capture->rot = val; 
        ui->label_rot->setText(QString::number(val) + "\xc2\xb0");
        });

    ui->AFcomboBox->setCurrentIndex(0);
    cam_option_changed_timer.setSingleShot(false);
    connect(&cam_option_changed_timer, &QTimer::timeout, this, [this]() {
        cam_option_changed_timer.stop();
        set_sensor_property();
        });

    connect(ui->box_auto_exposure, &QCheckBox::clicked, this, [this]() {
        cam_option_changed |= 0x0001;
        if (!cam_option_changed_timer.isActive())cam_option_changed_timer.start(100);
        });
    connect(ui->box_white, &QCheckBox::clicked, this, [this]() {
        cam_option_changed |= 0x0010;
        if (!cam_option_changed_timer.isActive())cam_option_changed_timer.start(100);
        });
    connect(ui->AFcomboBox, &QComboBox::activated, this, [this]() {
        cam_option_changed |= 0x0002;
        if(!cam_option_changed_timer.isActive())cam_option_changed_timer.start(100);
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
    connect(capture, &Capture::fpsReady, this, [this](double fps) {
        ui->fps_label->setText(QString::number(fps, 'f', 2) + "fps");
        });
    connect(ui->pushButton, &QPushButton::clicked, this, [this]() {
        ui->sliderCamOfs->setValue(0);
        signalProcess->cam_ofs = 0;
        ui->labelCamOfs->setText("0.0ms");
        });
    connect(ui->button_rot, &QPushButton::clicked, this, [this]() {
        ui->scroll_rot->setValue(0);
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

    refresh_plot_timer->start(20);
    splash.showMessage("Init capture");
    splash.showMessage("Done");
}

void MainWindow::set_sensor_property() {
    if ((cam_option_changed & 0x0001) != 0) { //auto
        if (ui->box_auto_exposure->isChecked()) {
            sensor.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, 1);
        }
        else {
            sensor.set_option(RS2_OPTION_EXPOSURE, ui->slider_exposure->value());
            sensor.set_option(RS2_OPTION_GAIN, ui->gainSlider->value());
        }
    }
    if ((cam_option_changed & 0x0002) != 0) { //powerline
        sensor.set_option(RS2_OPTION_POWER_LINE_FREQUENCY, ui->AFcomboBox->currentIndex());
    }
    if ((cam_option_changed & 0x0004) != 0) { //exposure
        if (!ui->box_auto_exposure->isChecked()){
            sensor.set_option(RS2_OPTION_EXPOSURE, ui->slider_exposure->value());
            }
    }
    if ((cam_option_changed & 0x0008) != 0) { //gain
        if (!ui->box_auto_exposure->isChecked())
            sensor.set_option(RS2_OPTION_GAIN, ui->gainSlider->value());
    }
    if ((cam_option_changed & 0x0010) != 0) { //auto white balance
        if (ui->box_white->isChecked()) {
            sensor.set_option(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE, 1);
        }
        else {
            sensor.set_option(RS2_OPTION_WHITE_BALANCE, ui->slider_white->value() * 10);
        }
    }
    if ((cam_option_changed & 0x0020) != 0) { //gain
        if (!ui->box_white->isChecked()) {
            sensor.set_option(RS2_OPTION_WHITE_BALANCE, ui->slider_white->value() * 10);
        }
    }
    cam_option_changed = 0;
}

MainWindow::~MainWindow()
{
    ppg->stop_reading();
    respi->stop_reading();
    delete ui;
}

void MainWindow::refreshRealsenseDevices() {
    rs2::context ctx;
    // Using the context we can get all connected devices in a device list
    if (is_opened) {
        sensor.close();
        is_opened = false;
    }
    devices = ctx.query_devices();
    if (devices.size() == 0)
    {
        rs2::device_hub device_hub(ctx);
        device_hub.wait_for_device();
        devices = ctx.query_devices();
    }
    for (rs2::device device : devices) {
        std::string name = "Unknown Device";
        if (device.supports(RS2_CAMERA_INFO_NAME))
            name = device.get_info(RS2_CAMERA_INFO_NAME);

        // and the serial number of the device:
        std::string sn = "########";
        if (device.supports(RS2_CAMERA_INFO_SERIAL_NUMBER))
            sn = std::string("#") + device.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);
        comboBox_cameras->addItem(QString::fromStdString(name + " " + sn));
        comboBox_cameras->setCurrentIndex(-1);
    }
}

void MainWindow::on_actionrefreshRealsense_triggered() {
    ui->actionrefreshRealsense->setDisabled(true);
    comboBox_cameras->clear();
    comboBox_cameras->setCurrentIndex(-1);
    comboBox_profile->clear();
    comboBox_profile->setCurrentIndex(-1);
    comboBox_cameras->setDisabled(true);
    QThread* thread = QThread::create([this]() {
        refreshRealsenseDevices();
        });
    connect(thread, &QThread::finished, this, [this]() {
        comboBox_cameras->setCurrentIndex(-1);
        comboBox_cameras->setDisabled(false);
        ui->actionrefreshRealsense->setDisabled(false);
        });
    thread->start();
}

void MainWindow::on_actionstopSerial_triggered() {
    ui->actionstopSerial->setDisabled(true);
    QThread* thread = QThread::create([this]() {
        ppg->stop_reading();
        respi->stop_reading();
        });
    connect(thread, &QThread::finished, this, [this]() {
        ui->actionstopSerial->setDisabled(false);
        });
    thread->start();
}

void MainWindow::freshSerialDevices() {
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo& info : infos) {
        if (!ppg->isRunning() && ppg->setPortName(info.portName()) ||
            !respi->isRunning() && respi->setPortName(info.portName())) {
            continue;
        }
        else if (ppg->isRunning() && respi->isRunning()) {
            break;
        }
    }
}
void MainWindow::on_actionrefreshSerial_triggered() {
    ui->actionrefreshSerial->setDisabled(true);
    QThread* thread = QThread::create([this]() {
        freshSerialDevices();
        });
    connect(thread, &QThread::finished, this, [this]() {
        ui->actionrefreshSerial->setDisabled(false);
        });
    thread->start();
}


void MainWindow::setfft(QImage image) {
    ui->fftLabel->setPixmap(QPixmap::fromImage(image));
}
void MainWindow::on_actionRecord_triggered() {
    if (capture->isRecording) {
        if (record_timer->isActive())
            record_timer->stop();
        capture->isRecording = false;
        isRecording = false;
        ui->actionRecord->setIcon(style()->standardIcon(QStyle::SP_DialogYesButton));
        ui->actionRecord->setToolTip("Start Recording");
        saveSignals();
        spinRecordTime->setValue(spin_record_last_time);
        spinRecordTime->setDisabled(false);
    }
    else {
        ui->toolBarRC->setDisabled(true);
        ui->actionRecord->setIcon(style()->standardIcon(QStyle::SP_DialogNoButton));
        ui->actionRecord->setToolTip("Stop Recording");
        spinRecordTime->setDisabled(true);
        ui->boxCamOfs->setDisabled(true);
        ui->VideoBox->setDisabled(true);
        int time = spinRecordTime->value();
        spin_record_last_time = time;
        if (time == 0) {
            time = 30;
            spinRecordTime->setValue(30);
        }
        QThread* thread = QThread::create([this]() {
            auto ts = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            std::stringstream ss; ss << ts << "/";
            if (capture->save_path != NULL) free(capture->save_path);
            std::string fn_str = ss.str();
            char* fname = (char*)malloc(fn_str.size() + 1);
            strcpy(fname, fn_str.c_str());
            capture->save_path = fname;
            QDir().mkdir((std::string(PROJECT_ROOT_PATH"rec/") + ss.str()).c_str());
            isRecording = true;
            });
        connect(thread, &QThread::finished, this, [this]() {
            capture->isRecording = true;
            record_timer->start();
            ui->toolBarRC->setDisabled(false);
            });
        thread->start();
    }
}

void MainWindow::on_actionStartTrigger_triggered() {
    if (capture->isRunning) {
        ui->VideoBox->setDisabled(true);
        ui->toolBarRS->setDisabled(true);
        ui->actionRecord->setDisabled(true);
        ui->actionStartTrigger->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
        ui->actionrefreshRealsense->setDisabled(false);
        comboBox_cameras->setDisabled(false);
        comboBox_profile->setDisabled(false);
        ui->actionStartTrigger->setToolTip("Play");
        QThread* thread = QThread::create([this]() {
            capture->isRunning = false;
            capture->wait();
            });
        connect(thread, &QThread::finished, this, [this]() {
            ui->toolBarRS->setDisabled(false);
            });
        thread->start();
    }
    else {
        //check is runnable
        if (capture->isRecording) on_actionRecord_triggered();
        ui->actionRecord->setDisabled(false);
        capture->isRunning = true;
        ui->actionStartTrigger->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
        ui->actionStartTrigger->setToolTip("Stop");
        ui->actionrefreshRealsense->setDisabled(true);
        comboBox_cameras->setDisabled(true);
        comboBox_profile->setDisabled(true);
        capture->start();
        ui->VideoBox->setDisabled(false);
    }
}

//void MainWindow::capFinished() {
//    capStopped = true;
//    //ui->statusIndicatorLabel->setText("Stopped");
//    ui->sourceTab->setDisabled(false);
//    ui->sourceTab->setDisabled(false);
//
//    ui->changeRateLabel->setDisabled(false);
//    ui->alphaLabel->setDisabled(false);
//    ui->betaLabel->setDisabled(false);
//    ui->camDelayLabel->setDisabled(false);
//    ui->box_group->setDisabled(false);
//    if (setSource) {
//        setSource = false;
//        if (!capture->setCapture(ui->videoComboBox->currentText().toStdString())) {
//            ui->videoComboBox->removeItem(ui->videoComboBox->currentIndex());
//        }
//        else {
//            totalSourceFileCanBeIndexed++;
//            QFileInfo fileInfo(ui->videoComboBox->currentText());
//            QString tsFilename = fileInfo.absolutePath() + '/' + fileInfo.completeBaseName() + "_ts.npy";
//            if (ui->tsComboBox->findText(tsFilename) == -1)
//                ui->tsComboBox->addItem(tsFilename);
//            ui->tsComboBox->setCurrentIndex(ui->tsComboBox->findText(tsFilename));
//        }
//    }
//}