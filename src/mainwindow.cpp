#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "imageviewer.h"
#include<qdebug>
#include <QtWidgets>
#include <string>
#include <thread>

void MainWindow::refresh_plot() {
    static uchar ref_cnt = 0;
    static bool r_status = true , g_status=true, b_status = true;
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

    ui->plot_interp->xAxis->setRange(ts + signalProcess->cam_ofs, show_window_length, Qt::AlignRight);
    ui->plot_interp->xAxis2->setRange(ts, show_window_length, Qt::AlignRight);
    ui->plot_interp->yAxis->rescale();
    ui->plot_interp->yAxis2->rescale();
    ui->plot_ppg->yAxis2->rescale();
    ui->plot_interp->replot();
    ui->plot_ppg->replot();
}
void MainWindow::set_profile(int idx) {
    setCursor(Qt::WaitCursor);
    ui->toolBarRS->setDisabled(true);
    static QThread* th = NULL;
    if (th != NULL) {
        th->wait();
        th->deleteLater();
        th = NULL;
    }
    th = QThread::create([this,idx]() {
        if (is_opened) {
            sensor.close();
            is_opened = false;
        }

        if (comboBox_cameras->currentText() == "MSMF") {
            auto cam_idx = comboBox_sensor->currentIndex();
            auto resol = cam_list[cam_idx].resolutions[idx];
            auto fps = cam_list[cam_idx].frameRate[idx];
            capture->setCamera(cam_idx,resol.width(),resol.height(),fps);
        }
        else {
            profile = profiles[profiles_ofs[comboBox_stream->currentIndex()] + idx];
            rs2::video_stream_profile video_stream_profile = profile.as<rs2::video_stream_profile>();
            is_opened = true;
            sensor.open(profile);
            cam_option_changed = 0xFFFF;
            set_sensor_property();
            capture->setCapture(sensor, video_stream_profile.width(), video_stream_profile.height(), video_stream_profile.fps());
        }
        });
    connect(th, &QThread::finished, this, [this]() {
        ui->toolBarRS->setDisabled(false);
        ui->actionStartTrigger->setDisabled(false);
        setCursor(Qt::ArrowCursor);
        });
    th->start();

}

void MainWindow::plotCustomSerial(uint32_t signal, double ts) {
    ui->plot_ppg->graph(2)->addData(ts, (double)signal / 0xFFFFFFFF);
    if (isRecording) {
        serial_ts_rec.push_back(ts);
        serial_sig_rec.push_back(signal);
    }
    
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
    setCursor(Qt::WaitCursor);
    ui->toolBarRC->setDisabled(true);
    isRecording = false;
    static QThread* th = NULL;
    if (th != NULL) {
        th->wait();
        th->deleteLater();
        th = NULL;
    }
    th = QThread::create([this]() {
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
        cnpy::npy_save(fname + "serial_sig.npy", serial_sig_rec);
        serial_sig_rec.clear();
        cnpy::npy_save(fname + "serial_ts.npy", serial_ts_rec);
        serial_ts_rec.clear();
        capture->wait_for_rec_save();
        });
    connect(th, &QThread::finished, this, [this]() {
        ui->toolBarRC->setDisabled(false);
        ui->VideoBox->setDisabled(false);
        ui->boxCamOfs->setDisabled(false);
        setCursor(Qt::ArrowCursor);
        });
    th->start();
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
void MainWindow::detect_sensors(int idx) {
    setCursor(Qt::WaitCursor);
    comboBox_sensor->clear();
    comboBox_stream->clear();
    comboBox_profile->clear();
    ui->actionStartTrigger->setDisabled(true);

    static QThread* th = NULL;
    if (th != NULL) {
        th->wait();
        th->deleteLater();
        th = NULL;
    }
    th = QThread::create([this, idx]() {

    if (is_opened) {
        sensor.close();
        is_opened = false;
    }
    if (comboBox_cameras->currentText() == "MSMF") {
        for (auto& cam : cam_list) {
            comboBox_sensor->addItem(cam.name);
            comboBox_sensor->setCurrentIndex(-1);
        }
    }
    else {
        device = devices[idx];
        sensors = device.query_sensors();
        for (rs2::sensor sensor : sensors)
        {
            if (sensor.supports(RS2_CAMERA_INFO_NAME) &&
                (std::string("RGB Camera") == sensor.get_info(RS2_CAMERA_INFO_NAME) || std::string("Stereo Module") == sensor.get_info(RS2_CAMERA_INFO_NAME))) {
                comboBox_sensor->addItem(QString::fromStdString(sensor.get_info(RS2_CAMERA_INFO_NAME)));
                comboBox_sensor->setCurrentIndex(-1);


            }
        }
    }

    });
    connect(th, &QThread::finished, this, [this]() {

        if (comboBox_cameras->currentText() == "MSMF") {
            comboBox_stream->setFixedWidth(0);
            comboBox_sensor->setFixedWidth(200);
        }
        else {
            comboBox_stream->setFixedWidth(100);
            comboBox_sensor->setFixedWidth(100);
        }
        setCursor(Qt::ArrowCursor);
        });
    th->start();
}
void MainWindow::detect_profiles(int idx) {
    setCursor(Qt::WaitCursor);
    comboBox_stream->clear();
    comboBox_profile->clear();
    ui->actionStartTrigger->setDisabled(true);
    static int max_resol_idx;
    max_resol_idx = -1;
    static QThread* th = NULL;
    if (th != NULL) {
        th->wait();
        th->deleteLater();
        th = NULL;
    }
    th = QThread::create([this,idx]() {
        if (is_opened) {
            sensor.close();
            is_opened = false;
        }

        if (comboBox_cameras->currentText() == "MSMF") {
            int max_resol = 0;
            for (int i = 0; i < cam_list[idx].resolutions.length(); i++) {
                auto resol = cam_list[idx].resolutions[i];
                auto fps = cam_list[idx].frameRate[i];
                comboBox_profile->addItem(QString("%1*%2@%3FPS").arg(resol.width()).arg(resol.height()).arg(fps));
                comboBox_profile->setCurrentIndex(-1);
                if (max_resol < resol.width() * resol.height()) {
                    max_resol = resol.width() * resol.height();
                    max_resol_idx = i;
                }
            }
        }
        else {
            sensor = sensors[idx];
            if (std::string("RGB Camera") == sensor.get_info(RS2_CAMERA_INFO_NAME)) {
                is_sensor_color = true;
                ui->box_white->setText("Auto White Balance");
                ui->slider_white->setMaximum(650);
                ui->slider_white->setMinimum(280);
                ui->slider_white->setSingleStep(1);
                ui->slider_white->setValue(460);

                ui->gainSlider->setMinimum(0);
                ui->gainSlider->setMaximum(128);
                ui->gainSlider->setValue(10);

                ui->slider_exposure->setMinimum(1);
                ui->slider_exposure->setMaximum(1000);
                ui->slider_exposure->setValue(10);
            }
            else if (std::string("Stereo Module") == sensor.get_info(RS2_CAMERA_INFO_NAME)) {
                is_sensor_color = false;
                ui->box_white->setText("Disable Laser");
                ui->slider_white->setMaximum(360);
                ui->slider_white->setMinimum(0);
                ui->slider_white->setSingleStep(30);
                ui->slider_white->setValue(150);

                ui->gainSlider->setMinimum(16);
                ui->gainSlider->setMaximum(248);
                ui->gainSlider->setValue(16);

                ui->slider_exposure->setMinimum(1);
                ui->slider_exposure->setMaximum(100000);
                ui->slider_exposure->setValue(33000);
            }

            ui->AFcomboBox->setEnabled(is_sensor_color);

            std::vector<rs2::stream_profile> stream_profiles = sensor.get_stream_profiles();
            profiles.clear();
            stream_names.clear();
            profiles_ofs.clear();
            int i = 0;
            for (rs2::stream_profile stream_profile : stream_profiles)
            {
                rs2_stream stream_data_type = stream_profile.stream_type();
                std::string stream_name = stream_profile.stream_name();
                rs2::video_stream_profile video_stream_profile = stream_profile.as<rs2::video_stream_profile>();
                if (video_stream_profile.format() == rs2_format::RS2_FORMAT_BGR8 ||
                    video_stream_profile.format() == rs2_format::RS2_FORMAT_Y8 ||
                    video_stream_profile.format() == rs2_format::RS2_FORMAT_Z16) {
                    bool contain_stream_name = false;
                    for (auto str : stream_names) {
                        if (str == stream_name) {
                            contain_stream_name = true;
                            break;
                        }
                    }
                    if (!contain_stream_name) {
                        stream_names.push_back(stream_name);
                        comboBox_stream->addItem(QString::fromStdString(stream_name));
                        comboBox_stream->setCurrentIndex(-1);
                        profiles_ofs.push_back(i);
                    }
                    profiles.push_back(stream_profile);
                    i++;
                }
            }
        }
        });
    connect(th, &QThread::finished, this, [this]() {
        if (comboBox_stream->count() == 1) {
            comboBox_stream->setCurrentIndex(0);
            set_stream(0);
        }
        else if (max_resol_idx != -1) {
            comboBox_profile->setCurrentIndex(max_resol_idx);
            this->set_profile(max_resol_idx);
        }
        setCursor(Qt::ArrowCursor);
        });
    th->start();
}


void MainWindow::set_stream(int idx) {
    setCursor(Qt::WaitCursor);
    comboBox_profile->clear();
    ui->actionStartTrigger->setDisabled(true);
    static QThread* th = NULL;
    if (th != NULL) {
        th->wait();
        th->deleteLater();
        th = NULL;
    }
    static int max_idx;
    max_idx = -1;
    th = QThread::create([this, idx]() {
        int add_count = 0;
        for (rs2::stream_profile profile : profiles)
        {
            rs2_stream stream_data_type = profile.stream_type();
            std::string stream_name = profile.stream_name();
            rs2::video_stream_profile video_stream_profile = profile.as<rs2::video_stream_profile>();
            int max_resol = 0;
            if (stream_name == stream_names[idx]) {
                comboBox_profile->addItem(QString::fromStdString((std::ostringstream() << video_stream_profile.width() << "x" << video_stream_profile.height() << "@" << video_stream_profile.fps() << "Hz").str()));
                int resol = video_stream_profile.width() * video_stream_profile.height();
                if (max_resol < resol) {
                    max_resol = resol;
                    max_idx = add_count;
                }
                add_count++;
            }
        }

        if (stream_names[idx] == "Depth") {
            is_profile_depth = true;
            ui->box_white->setDisabled(true);
            ui->box_white->setChecked(true);
            ui->slider_white->setDisabled(true);
        }
        else {
            is_profile_depth = false;
            ui->box_white->setDisabled(false);
            ui->slider_white->setDisabled(true);
        }
        });

    connect(th, &QThread::finished, this, [this]() {
        if (max_idx != -1) {
            int true_idx = comboBox_profile->count() - max_idx - 1;//UnKnown bug ??
            comboBox_profile->setCurrentIndex(true_idx);
            this->set_profile(true_idx);
        }
        setCursor(Qt::ArrowCursor);
        });
    th->start();
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
    comboBox_sensor = new QComboBox(ui->toolBarRS);
    comboBox_profile = new QComboBox(ui->toolBarRS);
    comboBox_stream = new QComboBox(ui->toolBarRS);
    comboBox_cameras->setFixedWidth(225);
    comboBox_sensor->setFixedWidth(100);
    comboBox_profile->setFixedWidth(130);
    comboBox_stream->setFixedWidth(100);
    ui->toolBarRS->addWidget(comboBox_cameras);
    ui->toolBarRS->addWidget(comboBox_sensor);
    ui->toolBarRS->addWidget(comboBox_stream);
    ui->toolBarRS->addWidget(comboBox_profile);
    connect(comboBox_cameras, &QComboBox::activated, this, &MainWindow::detect_sensors);
    connect(comboBox_sensor, &QComboBox::activated, this, &MainWindow::detect_profiles);
    connect(comboBox_stream, &QComboBox::activated, this, &MainWindow::set_stream);
    connect(comboBox_profile, &QComboBox::activated, this, &MainWindow::set_profile);
    ui->actionStartTrigger->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    ui->actionRecord->setIcon(style()->standardIcon(QStyle::SP_DialogYesButton));
    ui->actionrefreshRealsense->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
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
    spinRecordTime->setMaximum(9999);
    spinRecordTime->setMinimum(0);
    spinRecordTime->setValue(9999);
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

    ui->plot_ppg->addGraph(ui->plot_ppg->xAxis, ui->plot_ppg->yAxis2);
    ui->plot_ppg->graph(2)->setName("SERIAL");
    ui->plot_ppg->graph(2)->setPen(QPen(QColor(10, 255, 40)));
    ui->plot_ppg->yAxis2->setVisible(false);
    ui->plot_ppg->graph(0)->removeFromLegend();
    ui->plot_ppg->graph(1)->removeFromLegend();
    ui->plot_ppg->graph(2)->removeFromLegend();


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

    custom_serial = new CustomSerialReader();
    connect(custom_serial, &CustomSerialReader::serialReady, this, &MainWindow::plotCustomSerial);
    
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
        if (checked)
            ui->actionTracking->setText("Tracking");
        else
            ui->actionTracking->setText("Untracked");
        });
    connect(ui->slider_exposure, &QSlider::valueChanged, this, [this](int val) {
        ui->label->setText(QString::number(val));
        cam_option_changed |= 0x0004;
        if (!cam_option_changed_timer.isActive())cam_option_changed_timer.start(100);
        });
    connect(ui->slider_white, &QSlider::valueChanged, this, [this](int val) {
        if (is_sensor_color) {
            ui->label_6->setText(QString::number(val * 10) + "k");
            cam_option_changed |= 0x0020;
            if (!cam_option_changed_timer.isActive())cam_option_changed_timer.start(100);
        }
        else {
            ui->label_6->setText(QString::number(val * 10) + "k");
            cam_option_changed |= 0x0020;
            if (!cam_option_changed_timer.isActive())cam_option_changed_timer.start(100);
        }
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
    connect(capture, &Capture::fpsReady, this, [this](double fps,int height,int width) {
        ui->fps_label->setText(QString("%1*%2@%3").arg(width).arg(height).arg(fps, 0, 'f', 1));
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
    connect(ui->pushButton_sync, &QPushButton::clicked, this, [this](bool checked) {
        capture->is_syncing = checked;
        });
    connect(capture, &Capture::tsofsReady, this, &MainWindow::set_time_offset);
    
    ui->VideoBox->setDisabled(true);
    if(!QDir("./rec").exists())
        QDir().mkdir("./rec");
    //connect(capture->signalProcess, &SignalProcess::signalReady, this, &MainWindow::addSignal);

    //connect(capture, &Capture::finished, this, &MainWindow::on_stopButton_clicked);
    //connect(capture, &Capture::finished, this, &MainWindow::capFinished);

    refresh_plot_timer->start(20);
    connect(capture, &Capture::cap_started, this, [this]() {
        setCursor(Qt::ArrowCursor);
        });
    splash.showMessage("Init capture");
    splash.showMessage("Done");
}

void MainWindow::select_serial(int idx) {
    custom_serial->stop_reading();
    if (!custom_serial->setPort(serial_devices[idx])) {
        comboBox_serial->setCurrentIndex(-1);
    }
}

void MainWindow::set_time_offset(double ts_ofs) {
    ui->sliderCamOfs->setValue(int(ts_ofs * 10000));
    ui->pushButton_sync->setChecked(false);
    signalProcess->cam_ofs = ts_ofs;
    capture->is_syncing = false;
    ui->labelCamOfs->setText(QString::number((float)ts_ofs * 1000, 'f', 1) + "ms");
}

void MainWindow::set_sensor_property() {
    if (comboBox_cameras->currentText() == "MSMF") {
        if (cam_option_changed == 0)
            capture->setCVCamProperty(cv::CAP_PROP_BUFFERSIZE, 3);

        if ((cam_option_changed & 0x0002) != 0) { //powerline
            capture->setCVCamProperty(cv::CAP_PROP_BUFFERSIZE, 3);
        }
        if ((cam_option_changed & 0x0010) != 0) { //auto white balance
            if (ui->box_white->isChecked()) {
                capture->setCVCamProperty(cv::CAP_PROP_AUTO_WB, 1);
            }
            else {
                capture->setCVCamProperty(cv::CAP_PROP_AUTO_WB, 0);
                capture->setCVCamProperty(cv::CAP_PROP_WB_TEMPERATURE, ui->slider_white->value() * 10);
            }
        }
        if ((cam_option_changed & 0x0001) != 0) { //auto exposure gain
            if (ui->box_auto_exposure->isChecked()) {
                capture->setCVCamProperty(cv::CAP_PROP_AUTO_EXPOSURE, 1);
            }
            else {
                capture->setCVCamProperty(cv::CAP_PROP_EXPOSURE, ui->slider_exposure->value());
                capture->setCVCamProperty(cv::CAP_PROP_GAIN, ui->gainSlider->value());
            }
        }

        if ((cam_option_changed & 0x0004) != 0) { //exposure
            if (!ui->box_auto_exposure->isChecked()) {
                capture->setCVCamProperty(cv::CAP_PROP_AUTO_EXPOSURE, ui->slider_exposure->value());
            }
        }
        if ((cam_option_changed & 0x0020) != 0) { //wb
            if (!ui->box_white->isChecked())
                capture->setCVCamProperty(cv::CAP_PROP_WB_TEMPERATURE, ui->slider_white->value() * 10);
        }
    }
    else {
        if(cam_option_changed == 0)
            sensor.set_option(RS2_OPTION_FRAMES_QUEUE_SIZE, 1);

        if (is_sensor_color) {
            sensor.set_option(RS2_OPTION_AUTO_EXPOSURE_PRIORITY, 0);

            if ((cam_option_changed & 0x0002) != 0) { //powerline
                sensor.set_option(RS2_OPTION_POWER_LINE_FREQUENCY, ui->AFcomboBox->currentIndex());
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
        }
        else {
            if ((cam_option_changed & 0x0010) != 0 && !is_profile_depth) { //disable Laser
                if (ui->box_white->isChecked()) {
                    sensor.set_option(RS2_OPTION_EMITTER_ENABLED, 0);
                }
                else {
                    sensor.set_option(RS2_OPTION_EMITTER_ENABLED, 1);
                    sensor.set_option(RS2_OPTION_LASER_POWER, ui->slider_white->value());
                }

                if ((cam_option_changed & 0x0020) != 0) { //Laser
                    if (!ui->box_white->isChecked()) {
                        sensor.set_option(RS2_OPTION_LASER_POWER, ui->slider_white->value());
                    }
                }
            }
        }

        if ((cam_option_changed & 0x0001) != 0) { //auto
            if (ui->box_auto_exposure->isChecked()) {
                sensor.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, 1);
            }
            else {
                sensor.set_option(RS2_OPTION_EXPOSURE, ui->slider_exposure->value());
                sensor.set_option(RS2_OPTION_GAIN, ui->gainSlider->value());
            }
        }

        if ((cam_option_changed & 0x0004) != 0) { //exposure
            if (!ui->box_auto_exposure->isChecked()) {
                sensor.set_option(RS2_OPTION_EXPOSURE, ui->slider_exposure->value());
            }
        }
        if ((cam_option_changed & 0x0008) != 0) { //gain
            if (!ui->box_auto_exposure->isChecked())
                sensor.set_option(RS2_OPTION_GAIN, ui->gainSlider->value());
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
    // do not block
    //if (devices.size() == 0)
    //{
    //    rs2::device_hub device_hub(ctx);
    //    device_hub.wait_for_device();
    //    devices = ctx.query_devices();
    //}
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

    cam_list = get_camera_map();
    comboBox_cameras->addItem(QString("MSMF"));
    comboBox_cameras->setCurrentIndex(-1);
}

void MainWindow::on_actionrefreshRealsense_triggered() {
    setCursor(Qt::WaitCursor);
    ui->actionrefreshRealsense->setDisabled(true);
    comboBox_cameras->clear();
    comboBox_cameras->setCurrentIndex(-1);
    comboBox_profile->clear();
    comboBox_profile->setCurrentIndex(-1);
    comboBox_stream->clear();
    comboBox_stream->setCurrentIndex(-1);
    comboBox_sensor->clear();
    comboBox_sensor->setCurrentIndex(-1);
    comboBox_cameras->setDisabled(true);
    static QThread* th = NULL;
    if (th != NULL) {
        th->wait();
        th->deleteLater();
        th = NULL;
    }
    th = QThread::create([this]() {
        refreshRealsenseDevices();
        });
    connect(th, &QThread::finished, this, [this]() {
        comboBox_cameras->setCurrentIndex(-1);
        comboBox_cameras->setDisabled(false);
        ui->actionrefreshRealsense->setDisabled(false);
        setCursor(Qt::ArrowCursor);
        });
    th->start();
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
    comboBox_serial->setCurrentIndex(-1);
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
        setCursor(Qt::ArrowCursor);
        });
    th->start();
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
        MessageBeep(MB_OK);
        spinRecordTime->setValue(spin_record_last_time);
        spinRecordTime->setDisabled(false);
        filenameLineEdit->setDisabled(false);
    }
    else {
        setCursor(Qt::WaitCursor);
        ui->toolBarRC->setDisabled(true);
        ui->actionRecord->setIcon(style()->standardIcon(QStyle::SP_DialogNoButton));
        ui->actionRecord->setToolTip("Stop Recording");
        spinRecordTime->setDisabled(true);
        ui->boxCamOfs->setDisabled(true);
        filenameLineEdit->setDisabled(true);
        ui->VideoBox->setDisabled(true);
        int time = spinRecordTime->value();
        spin_record_last_time = time;
        if (time == 0) {
            time = 30;
            spinRecordTime->setValue(30);
        }
        static QThread* th = NULL;
        if (th != NULL) {
            th->wait();
            th->deleteLater();
            th = NULL;
        }
        th = QThread::create([this]() {
            auto ts = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            std::stringstream ss;
            if(filenameLineEdit->text().length() !=0 )
                ss << ts <<"_" << filenameLineEdit->text().toStdString() << "/";
            else
                ss << ts << "/";
            if (capture->save_path != NULL) free(capture->save_path);
            std::string fn_str = ss.str();
            char* fname = (char*)malloc(fn_str.size() + 1);
            strcpy(fname, fn_str.c_str());
            capture->save_path = fname;
            QDir().mkdir((std::string("./rec/") + ss.str()).c_str());
            isRecording = true;
            });
        connect(th, &QThread::finished, this, [this]() {
            capture->isRecording = true;
            record_timer->start();
            ui->toolBarRC->setDisabled(false);
            setCursor(Qt::ArrowCursor);
            });
        th->start();
    }
}

void MainWindow::on_actionStartTrigger_triggered() {
    if (capture->isRunning) {
        setCursor(Qt::WaitCursor);
        ui->VideoBox->setDisabled(true);
        ui->toolBarRS->setDisabled(true);
        ui->actionStartTrigger->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
        ui->actionrefreshRealsense->setDisabled(false);
        comboBox_cameras->setDisabled(false);
        comboBox_profile->setDisabled(false);
        comboBox_stream->setDisabled(false);
        comboBox_sensor->setDisabled(false);
        ui->actionStartTrigger->setToolTip("Play");
        static QThread* th = NULL;
        if (th != NULL) {
            th->wait();
            th->deleteLater();
            th = NULL;
        }
        th = QThread::create([this]() {
            capture->isRunning = false;
            capture->wait();
            });
        connect(th, &QThread::finished, this, [this]() {
            ui->toolBarRS->setDisabled(false);
            setCursor(Qt::ArrowCursor);
            }); //Qt automatically disconnected when one of the class is deleted
        th->start();
    }
    else {
        //check is runnable
        setCursor(Qt::WaitCursor);
        if (capture->isRecording) on_actionRecord_triggered();
        capture->isRunning = true;
        ui->actionStartTrigger->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
        ui->actionStartTrigger->setToolTip("Stop");
        ui->actionrefreshRealsense->setDisabled(true);
        comboBox_cameras->setDisabled(true);
        comboBox_sensor->setDisabled(true);
        comboBox_profile->setDisabled(true);
        comboBox_stream->setDisabled(true);
        capture->start();
        ui->VideoBox->setDisabled(false); 
        // connection created above
    }
}