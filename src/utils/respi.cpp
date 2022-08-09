#include "respi.h"
#include <qdebug>
#include <qtimer>
#include "serialport.h"
using namespace serial;
#define fs 50
RESPIReader::RESPIReader()
    :serial_reader("",9600)
{
}

bool RESPIReader::setPort(const serial::PortInfo& device) {
    if (device.description.find("Silicon Labs CP210x USB to UART Bridge") != std::string::npos) {
        return setPortName(device.port);
    }
    else
        return false;
}
bool RESPIReader::setPortName(const std::string& portName) {
    std::string read_data;
    uint32_t parsed_data;
    if (serial_reader.isOpen())
        serial_reader.close();
    try {
        serial_reader.setPort(portName.c_str());
        serial_reader.setTimeout(100, 20, 1, 20, 1);
        serial_reader.open();
    }
    catch (...) {
        return false;
    }
    if (!serial_reader.isOpen()) {
        return false;
    }
    serial_reader.setTimeout(100, 20, 1, 20, 1);
    serial_reader.write("\x20\x33");
    serial_reader.flushInput();
    read_data = serial_reader.readline(100, "\x20\x33");
    if (read_data.find_last_of("\x20\x33") != read_data.size() - 1) {
        goto FALSE_RETURN;
    }
    msleep(10);
    serial_reader.write("\x20\x31");
    serial_reader.flushInput();
    read_data = serial_reader.read(6);
    if (read_data.size() != 6) {
        goto FALSE_RETURN;
    }
    if ((uchar)read_data[0]!=0x20 || (uchar)read_data[1] != 0x31) {
        goto FALSE_RETURN;
    }
    start_reading();
    return true;

FALSE_RETURN:
    if (serial_reader.isOpen())
        serial_reader.close();
    return false;
}

void RESPIReader::run() {
    string read_data;
    int error_num = 0;
    msleep(10);
    serial_reader.write("\x20\x32");
    serial_reader.flush();
    serial_reader.setTimeout(20, 20, 0, 20, 0);
    try {
        while (true) {
            read_data = serial_reader.read(1);
            if (read_data.length() == 1) {
                error_num = 0;
                double time_ofs = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count() - 1.0 / fs * serial_reader.available();
                emit respiReady((uchar)read_data[0], time_ofs);
            }
            else {
                error_num++;
            }
            if (error_num == 10 || stop_read_sig == true)
                break;
        }
        serial_reader.setTimeout(100, 100, 0, 100, 0);
        serial_reader.write("\x20\x33");
        serial_reader.flush();
        serial_reader.readline(100, "\x20\x33");
    }
    catch (IOException e) {
        serial_reader.close();
    }
}
bool RESPIReader::start_reading() {
    if (!serial_reader.isOpen() || this->isRunning())
        return false;
    stop_read_sig = false;
    start();
    return true;
}

bool RESPIReader::stop_reading() {
    if (this->isRunning()) {
        stop_read_sig = true;
        this->wait();
        return true;
    }
    else
        return false;
}