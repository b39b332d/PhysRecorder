#include "SerialReader.h"
#include <qdebug>
#include <qtimer>
#include <qcustomplot.h>
#include <cnpy.h>

#include <serialport.h>
using namespace serial;
#define UPDATE_RESOLUTION 0.03
extern double win_length;
SerialReader::SerialReader(unsigned int baudrate,const std::string &n)
    :serial_reader(new serial::Serial("", baudrate)),signal_name(n)
{
}
SerialReader::~SerialReader()
{
    delete serial_reader;
}
bool SerialReader::setPort(const serial::PortInfo& device) {
    return setPortName(device.port);
}

bool SerialReader::start_reading() {
    if (is_running)
        return false;
    stop_read_sig = false;

    if (th) {
        th->join();
        delete th;
    }
    is_running = true;
    th = new std::thread(&SerialReader::run_wrapper,this);
    return true;
}

bool SerialReader::stop_reading() {
    if (is_running) {
        stop_read_sig = true;
        th->join();
        th = nullptr;
        delete th;
        return true;
    }
    else
        return false;
}

void SerialReader::serial_add_signal(double s, double t)
{
    emit serial_ready(s, t);
    if (graph) {
        if (t - last_ts > UPDATE_RESOLUTION) {
            graph->addData(t, s);
            graph->data()->removeBefore(t - win_length);
            last_ts = t;
        }
    }
    std::unique_lock l(serial_lock);
    if (is_recording) {
        signal_buffer.push_back(s);
        ts_buffer.push_back(t);
        if (ts_buffer.size() >= 4095) {
            save_signal_async();
        }
    }
}
void SerialReader::save_signal_async()
{
    if (ts_buffer.empty())
        return;
    if (save_thread) {
        save_thread->join();
        delete save_thread;
        save_thread = nullptr;
    }
    save_thread = new std::thread([this](std::vector<double> signal_buffer_temp,
        std::vector<double> ts_buffer_temp) {
            cnpy::npy_save<double>(save_filename + "_sig.npy",
                &(signal_buffer_temp[0]), { signal_buffer_temp.size() }, "a");
            cnpy::npy_save<double>(save_filename + "_ts.npy",
                &(ts_buffer_temp[0]), { ts_buffer_temp.size() }, "a");
        }, signal_buffer, ts_buffer);

    signal_buffer.clear();
    ts_buffer.clear();
}

void SerialReader::startRecording(std::string filename)
{
    std::unique_lock l(serial_lock);
    save_filename = filename;
    is_recording = true;
}

void SerialReader::stopRecording()
{
    serial_lock.lock();
    is_recording = false;

    save_signal_async();

    serial_lock.unlock();
    if (save_thread) {
        save_thread->join();
        delete save_thread;
        save_thread = nullptr;
    }

}


void SerialReader::invalidDevice()
{
    if (is_recording) {
        stopRecording();
    }
    is_invalid = true;
}




void SerialReader::onSerialStop()
{
    // maybe need lock
    if (serial_reader->isOpen())
        serial_reader->close();
    is_running = false;
    emit serial_stopped(this);
}
#include "ppg.h"
#include "respi.h"
#include "custom_serial.h"
#include <unordered_set>
// map key is unique name ; do not use ; use val.device_name instead
std::map<std::string, SerialReader*> SerialReader::serial_readers;
void SerialReader::refresh_serials()
{
    SerialReader* serial_reader;
    std::vector<serial::PortInfo>devices_found = serial::list_ports();
    std::unordered_set<std::string> devices_found_names;
    for (const auto& dev : devices_found) {
        std::string dev_name = dev.hardware_id+dev.port;
        devices_found_names.insert(dev_name);
        auto it = serial_readers.find(dev_name);
        if (it != serial_readers.end())
            if(!it->second->is_invalid)
            continue;
            else {
                delete it->second;
                serial_readers.erase(it);
            }
        if (dev.description.find("Silicon Labs CP210x USB to UART Bridge") != std::string::npos) {

            serial_reader = new RESPIReader();
            if (serial_reader->setPort(dev)) {
                serial_readers[dev_name]=serial_reader;
                goto find_paired;
            }
            else delete serial_reader;

            serial_reader = new PPGReader();
            if (serial_reader->setPort(dev)) {
                serial_readers[dev_name] = serial_reader;
                goto find_paired;
            }
            else delete serial_reader;
        }
        else{
            serial_reader = new CustomSerialReader();
            if (serial_reader->setPort(dev)) {
                serial_readers[dev_name] = serial_reader;
                goto find_paired;
            }
            else delete serial_reader;
        }
        continue;
    find_paired:
        serial_reader->device_name = serial_reader->signal_name + "-" + dev.port;
        serial_reader->friendly_name = serial_reader->device_name;
    }
    auto iter = serial_readers.begin();
    while (iter!= serial_readers.end()) {
        auto c_it = iter;
        iter++;
        if (devices_found_names.find(c_it->first) == devices_found_names.end()) {
            delete c_it->second;
            serial_readers.erase(c_it);
        }
    }
}
