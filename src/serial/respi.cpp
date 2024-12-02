#include "respi.h"
#include <qdebug>
#include <qtimer>
#include "serialport.h"
using namespace serial;
#define respi_fs 50
RESPIReader::RESPIReader()
    :SerialReader(9600,"Respi")
{
}

bool RESPIReader::setPortName(const std::string& portName) {
    std::string read_data;
    uint32_t parsed_data;
    if (serial_reader->isOpen())
        serial_reader->close();
    try {
        serial_reader->setPort(portName.c_str());
        serial_reader->setTimeout(100, 20, 1, 20, 1);
        serial_reader->open();
    }
    catch (...) {
        return false;
    }
    if (!serial_reader->isOpen()) {
        return false;
    }
    serial_reader->setTimeout(100, 20, 1, 20, 1);
    serial_reader->write("\x20\x33");
    serial_reader->flushInput();
    read_data = serial_reader->readline(100, "\x20\x33");
    if (read_data.find_last_of("\x20\x33") != read_data.size() - 1) {
        goto FALSE_RETURN;
    }
    Sleep(10);
    serial_reader->write("\x20\x31");
    serial_reader->flushInput();
    read_data = serial_reader->read(6);
    if (read_data.size() != 6) {
        goto FALSE_RETURN;
    }
    if ((uchar)read_data[0]!=0x20 || (uchar)read_data[1] != 0x31) {
        goto FALSE_RETURN;
    }
    //start_reading();
    serial_reader->close();
    return true;

FALSE_RETURN:
    if (serial_reader->isOpen())
        serial_reader->close();
    return false;
}

void RESPIReader::run() {
    string read_data;
    int error_num = 0;
    try {
        serial_reader->open();
        Sleep(10);
        serial_reader->write("\x20\x32");
        serial_reader->flush();
        serial_reader->setTimeout(20, 20, 0, 20, 0);

        while (true) {
            read_data = serial_reader->read(1);
            if (read_data.length() == 1) {
                error_num = 0;
                double time_ofs = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count() - 1.0 / respi_fs * serial_reader->available();
                serial_add_signal((double)((uchar)read_data[0]) / 256, time_ofs);
            }
            else {
                error_num++;
            }
            if (error_num == 20)
                throw std::exception();
            if (stop_read_sig == true)
                break;
        }
        serial_reader->setTimeout(100, 100, 0, 100, 0);
        serial_reader->write("\x20\x33");
        serial_reader->flush();
        serial_reader->readline(100, "\x20\x33");
    }
    catch (std::exception e) {
        invalidDevice();
    }
    onSerialStop();
}