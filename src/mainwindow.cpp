#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QtWidgets>
#include <string>
#include <thread>
#include <QFile>
#include <QTextStream>
#include <qstandardpaths.h>
#include <windows.h>
#include <SerialReader.h>
#include <MultiSelectComboBox.h>
#include "signalprocess.h"
#include <video_ui.h>
double win_length=8;

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
void MainWindow::closeEvent(QCloseEvent* event)
{
    if (record_timer->isActive()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            tr("Close Application"),
            tr("Recording, Are you want to close the application?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );

        if (reply == QMessageBox::Yes) {
            // Perform any cleanup or saving operations
            // Accept the event - window will close
            event->accept();
        }
        else {
            // Ignore the event - window will stay open
            event->ignore();
        }
        return;
    }
    event->accept();
    return;
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

MainWindow::MainWindow(QSplashScreen& splash, QWidget* parent)
//use reference instead of pointer
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , worker(this)
{
    splash.showMessage("Setting up UI");
    ui->setupUi(this);

    this->setWindowTitle("Remote PhotoPlethysmoGraphy");

    ui->actionRecord->setIconSize(QSize(30, 30));
    ui->actionRecord->setIcon(style()->standardIcon(QStyle::SP_DialogYesButton));
    connect(ui->actionRecord, &QPushButton::clicked, this, &MainWindow::onRecordToggled);

    ui->actionrefreshSerial->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    ui->actionstopSerial->setIcon(style()->standardIcon(QStyle::SP_MediaStop));


    comboBox_serial = new MultiSelectComboBox(ui->toolBarSerial);
    comboBox_serial->setFixedWidth(150);
    ui->toolBarSerial->insertWidget(ui->actionstopSerial, comboBox_serial);
    connect(comboBox_serial, &MultiSelectComboBox::selectionChanged, this, &MainWindow::onSerialSelected);
    connect(comboBox_serial, &MultiSelectComboBox::highLightSelect, this, &MainWindow::onSerialHighted);


    textedit_serialname = new QLineEdit(ui->toolBarSerial);
    textedit_serialname->setMinimumWidth(150);
    textedit_serialname->setMaximumWidth(200);
    textedit_serialname->setDisabled(true);
    textedit_serialname->setPlaceholderText("Serial Filename");
    ui->toolBarSerial->addWidget(textedit_serialname);



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
    connect(signalProcess, &SignalProcess::fftReady, this, &MainWindow::setfft);

    splash.showMessage("Detecting Serial Device...");
    
    on_actionrefreshSerial_triggered();

    splash.showMessage("Connect signals");

    ui->actionR->setProperty("channel",  signalProcess->r_channel);
    ui->actionG->setProperty("channel", signalProcess->g_channel);
    ui->actionB->setProperty("channel", signalProcess->b_channel);
    ui->actionPOS->setProperty("channel", signalProcess->pos_channel);
    connect(ui->actionR, &QPushButton::toggled, signalProcess, &SignalProcess::setShowChannel);
    connect(ui->actionG, &QPushButton::toggled, signalProcess, &SignalProcess::setShowChannel);
    connect(ui->actionB, &QPushButton::toggled, signalProcess, &SignalProcess::setShowChannel);
    connect(ui->actionPOS, &QPushButton::toggled, signalProcess, &SignalProcess::setShowChannel);
    connect(ui->actionR, &QPushButton::toggled, [this](bool checked) {if(!checked) ui->actionR->setText("R"); });
    connect(ui->actionG, &QPushButton::toggled, [this](bool checked) {if (!checked)ui->actionG->setText("G"); });
    connect(ui->actionB, &QPushButton::toggled, [this](bool checked) {if (!checked)ui->actionB->setText("B"); });
    connect(ui->actionPOS, &QPushButton::toggled, [this](bool checked) {if (!checked)ui->actionPOS->setText("POS"); });

    connect(signalProcess, &SignalProcess::sqiReady, this, &MainWindow::setSqi);


    connect(ui->window_slider, &QSlider::valueChanged, this, [this](int val) {
        show_window_length = (double)val / 10;
        ui->label_window_len->setText(QString::number(show_window_length, 'f', 1)+"s");
        win_length = show_window_length;
        });

    connect(ui->sliderCamOfs, &QSlider::valueChanged, this, [this](int val) {
        signalProcess->cam_ofs = (double)val / 10000;
        ui->labelCamOfs->setText(QString::number((float)val/10, 'f', 1)+"ms");
        });

    connect(ui->pushButton, &QPushButton::clicked, this, [this]() {
        ui->sliderCamOfs->setValue(0);
        signalProcess->cam_ofs = 0;
        ui->labelCamOfs->setText("0.0ms");
        });
    
    refresh_plot_timer->start(50);

    sharedFilePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/phyrecorder_ipc.temp";

    watcher.addPath(sharedFilePath);
    QObject::connect(&watcher, &QFileSystemWatcher::fileChanged,this, &MainWindow::onFileChanged);

    splash.showMessage("Init capture");


	videoui = new VideoUI(ui, signalProcess,this);

    worker.wait();
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
    int h_serial = comboBox_serial->getHighLight();
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



void MainWindow::onSerialSelected(int idx,bool selected) {
    SerialReader* serial_reader = comboBox_serial->itemData(idx).value<SerialReader*>();
    setCursorBusy(true);
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
        setCursorBusy(false);
    }
    else {
        worker.run_with_call_back([this, serial_reader]() {
            serial_reader->disconnect(this);
            is_stop_successed = serial_reader->stop_reading();
            }, [this, serial_reader]() {
                if(is_stop_successed)
                    onSerialStopped(serial_reader);
                setCursorBusy(false);
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


void MainWindow::on_actionstopSerial_triggered() {
    setCursorBusy(true);
    ui->actionstopSerial->setDisabled(true);

    auto high_idx = comboBox_serial->getHighLight();
    if (high_idx == -1) {
        static QSet<SerialReader*> successed_stopped;
        worker.run_with_call_back([this]() {
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
                setCursorBusy(false);
         });
    }
    else {
        onSerialSelected(high_idx, false);
        ui->actionstopSerial->setDisabled(false);
        setCursorBusy(false);
    }
}
void MainWindow::on_actionrefreshSerial_triggered() {
    setCursorBusy(true);
    ui->actionrefreshSerial->setDisabled(true);
    int c_highidx = comboBox_serial->getHighLight();
    if(c_highidx !=-1)
        onSerialHighted(c_highidx, false);
    comboBox_serial->clear();

    worker.run_with_call_back([this]() {
        SerialReader::refresh_serials();
        }, 
    [this]() {
        for (auto& [dev_name, dev] : SerialReader::serial_readers) {
            comboBox_serial->addItem(QString::fromStdString(dev->device_name),
                QVariant::fromValue(dev),
                dev->is_running);
        }
        ui->actionrefreshSerial->setDisabled(false);
        setCursorBusy(false);
        });
}


void MainWindow::setfft(const QImage& image) {
    ui->fftLabel->setPixmap(QPixmap::fromImage(image));
}

std::string MainWindow::start_record(std::string save_prefix ="") {
    setCursorBusy(true);
    ui->toolBarRecord->setEnabled(false);
    ui->actionRecord->setIcon(style()->standardIcon(QStyle::SP_DialogNoButton));
    ui->actionRecord->setToolTip("Stop Recording");
    ui->spinRecordTime->setDisabled(true);
    ui->boxCamOfs->setDisabled(true);
    ui->filenameLineEdit->setDisabled(true);
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

    videoui->start_record(save_prefix);

    worker.run_with_call_back(
        [this, save_prefix]() {
            for (auto s : comboBox_serial->getSelectedItems()) {
                SerialReader* reader = comboBox_serial->itemData(s).value<SerialReader*>();
                reader->startRecording(save_prefix + reader->friendly_name);
                rec_SerialReaders.insert(reader);
            }
        },
        [this]() {
            record_timer->start();
            ui->toolBarRecord->setEnabled(true);
            setCursorBusy(false);
        }
    );
    return temp_ret;
}
void MainWindow::stop_record() {
    if (record_timer->isActive())
        record_timer->stop();
    setCursorBusy(true);
    ui->toolBarRecord->setEnabled(false);
    ui->spinRecordTime->setValue(spin_record_last_time);
    ui->filenameLineEdit->setText(filenameLineEdit_name);

    worker.run_with_call_back([this]() {
        for (auto s : rec_SerialReaders)  s->stopRecording();
            rec_SerialReaders.clear();
        videoui->stop_record();
        },
        [this]() {
            ui->actionRecord->setIcon(style()->standardIcon(QStyle::SP_DialogYesButton));
            ui->actionRecord->setToolTip("Start Recording");
            ui->toolBarRecord->setEnabled(true);
            ui->VideoBox->setDisabled(false);
            ui->boxCamOfs->setDisabled(false);
            ui->spinRecordTime->setDisabled(false);
            ui->filenameLineEdit->setDisabled(false);
            setCursorBusy(false);
        });
    MessageBeep(MB_OK);
    
}
void MainWindow::onRecordToggled() {
    if (record_timer->isActive()) {
        emitFileSignal(false);
        stop_record();
    }
    else {
        spin_record_last_time = ui->spinRecordTime->value();
        auto save_path = start_record();
        emitFileSignal(true,save_path.c_str());
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
            if (status == "__stop" && record_timer->isActive())
                stop_record();
            else if (status == "__start" && !record_timer->isActive()) {
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


__declspec(dllexport) void start_mainwin(QSplashScreen& screen) {
    MainWindow *w = new MainWindow(screen);
    w->show();
    screen.finish(w);
}