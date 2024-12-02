#include "custom_serial.h"
#include <qdebug>
#include <qtimer>
#include "serialport.h"
using namespace serial;
CustomSerialReader::CustomSerialReader()
    :SerialReader(115200,"Serial")
{
}

bool CustomSerialReader::setPortName(const std::string& portName) {
    std::string read_data;
    uint32_t parsed_data;
    if (serial_reader->isOpen())
        serial_reader->close();
    try {
        serial_reader->setPort(portName.c_str());
        serial_reader->setTimeout(100, 100, 0, 100, 0);
        serial_reader->open();
    }
    catch (...) {
        return false;
    }
    if (!serial_reader->isOpen()) {
        return false;
    }

    serial_reader->flush();
    read_data = serial_reader->readline(100, "\r\n");
    read_data = serial_reader->readline(100, "\r\n");
    if (read_data.size() <= 2) goto FALSE_RETURN;
    read_data.pop_back();
    read_data.pop_back();
    if (read_data.find_first_not_of("0123456789") != std::string::npos) {
        goto FALSE_RETURN;
    }
    //serial_reader->write("\xff\xcb\x04\xa5\xa4\xfd");
    //serial_reader->flushInput();
    //read_data = serial_reader->read(5);

    //start_reading();
    serial_reader->close();
    return true;

FALSE_RETURN:
    if (serial_reader->isOpen())
        serial_reader->close();
    return false;
}

void CustomSerialReader::run() {
    string read_data;
    int error_num = 0;
    try {
        serial_reader->open();
        Sleep(2);
        serial_reader->flush();
        serial_reader->setTimeout(50, 50, 0, 5, 0);

        while (true) {
            read_data = serial_reader->readline(30, "\r\n");
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
                serial_add_signal((double)parsed_data / 0xFFFFFFFF, time_ofs);
            }
            if (error_num == 20)
                throw std::exception();
            if (stop_read_sig == true)
                break;
        }
    }
    catch (std::exception e) {
        invalidDevice();
    }
    onSerialStop();
}