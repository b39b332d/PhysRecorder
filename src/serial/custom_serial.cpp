#include "custom_serial.h"
#include <qdebug>
#include <qtimer>
#include "serialport.h"
using namespace serial;
#define fs 200
CustomSerialReader::CustomSerialReader()
    :serial_reader("", 115200)
{
}

bool CustomSerialReader::setPort(const serial::PortInfo& device) {
    return setPortName(device.port);
}
bool CustomSerialReader::setPortName(const std::string& portName) {
    std::string read_data;
    uint32_t parsed_data;
    if (serial_reader.isOpen())
        serial_reader.close();
    try {
        serial_reader.setPort(portName.c_str());
        serial_reader.setTimeout(100, 100, 0, 100, 0);
        serial_reader.open();
    }
    catch (...) {
        return false;
    }
    if (!serial_reader.isOpen()) {
        return false;
    }

    serial_reader.flush();
    read_data = serial_reader.readline(100, "\r\n");
    read_data = serial_reader.readline(100, "\r\n");
    read_data.pop_back();
    read_data.pop_back();
    if (read_data.size() == 0) {
        goto FALSE_RETURN;
    }
    if (read_data.find_first_not_of("0123456789") != std::string::npos) {
        goto FALSE_RETURN;
    }
    //serial_reader.write("\xff\xcb\x04\xa5\xa4\xfd");
    //serial_reader.flushInput();
    //read_data = serial_reader.read(5);

    start_reading();
    return true;

FALSE_RETURN:
    if (serial_reader.isOpen())
        serial_reader.close();
    return false;
}

void CustomSerialReader::run() {
    string read_data;
    int error_num = 0;
    serial_reader.flush();
    serial_reader.setTimeout(50, 50, 0, 5, 0);
    try {
        while (true) {
            read_data = serial_reader.readline(30, "\r\n");
            double time_ofs = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
            if (read_data.size() <=2) {
                //if (read_data.size() == 7)
                //    qDebug() << (uchar)read_data[0] << (uchar)read_data[1] << (uchar)read_data[2] << (uchar)read_data[3] << (uchar)read_data[4] << (uchar)read_data[5] << (uchar)read_data[6];
                error_num++;
            }
            else {
                read_data.pop_back();
                read_data.pop_back();
                uint32_t parsed_data = std::stoul(read_data);
                emit serialReady(parsed_data, time_ofs);
            }
            if (error_num == 20 || stop_read_sig == true)
                break;
        }
    }
    catch (IOException e) {
        serial_reader.close();
    }
}
bool CustomSerialReader::start_reading() {
    if (!serial_reader.isOpen() || this->isRunning())
        return false;
    stop_read_sig = false;
    start();
    return true;
}

bool CustomSerialReader::stop_reading() {
    if (this->isRunning()) {
        stop_read_sig = true;
        this->wait();
        return true;
    }
    else
        return false;
}