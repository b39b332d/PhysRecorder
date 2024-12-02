#include "ppg.h"
#include <qdebug>
#include <qtimer>
#include "serialport.h"
using namespace serial;
#define ppg_fs 200
PPGReader::PPGReader()
    :SerialReader(115200,"PPG")
{
}

uint32_t validate_data(std::string& data) {
    if (data.length() < 5)
        return 0xFFFFFFFF;
    uchar sum = (uchar)data[0] + (uchar)data[2];
    uint32_t out = 0;
    for (int i = 3; i < data.length(); i++) {
        sum += (uchar)data[i];
        out <<= 8;
        out |= (uchar)data[i];
    }
    if (sum == (uchar)data[1])
        return out;
    return 0xFFFFFFFF;
}
bool PPGReader::setPortName(const std::string& portName) {
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
    serial_reader->write("\xff\xcb\x03\xa4\xa1");
    serial_reader->flushInput();
    read_data = serial_reader->readline(100, "\xff\xcb\x03\xa4\xa1");
    if (read_data.find_last_of("\xff\xcb\x03\xa4\xa1") != read_data.size() - 1) {
        goto FALSE_RETURN;
    } 
    Sleep(2);
    serial_reader->write("\xff\xcb\x03\xa5\xa2");
    serial_reader->flushInput();
    read_data = serial_reader->read(9);
    if (read_data.size() != 9) {
        goto FALSE_RETURN;
    }
    parsed_data = validate_data(read_data.erase(0,2));
    if (parsed_data == 0xFFFFFFFF || parsed_data<0x00010000) {
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

void PPGReader::run() {
    string read_data;
    int error_num = 0;
    try {
        serial_reader->open();
        Sleep(2);
        serial_reader->write("\xff\xcb\x03\xa3\xa0");
        serial_reader->flush();
        serial_reader->setTimeout(5, 5, 0, 5, 0);

        while (true) {
            read_data = serial_reader->readline(7, "\xff\xcb");
            double time_ofs = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count() - 1.0 / ppg_fs * ((-5 + serial_reader->available()) / 7 + 1);
            if (read_data.size() != 7 || (uchar)read_data[5] != 0xff || (uchar)read_data[6] != 0xcb) {
                //if (read_data.size() == 7)
                //    qDebug() << (uchar)read_data[0] << (uchar)read_data[1] << (uchar)read_data[2] << (uchar)read_data[3] << (uchar)read_data[4] << (uchar)read_data[5] << (uchar)read_data[6];
                error_num++;
            }
            else {
                uint32_t parsed_data = validate_data(read_data.erase(5, 2));
                if (parsed_data != 0xFFFFFFFF) {
                    error_num = 0;
                    serial_add_signal((double)((uint16_t)parsed_data) / 1024, time_ofs);
                }
                else {
                    error_num++;
                }
            }
            if (error_num == 50)
                throw std::exception();
            if (stop_read_sig == true)
                break;
        }
        serial_reader->write("\xff\xcb\x03\xa4\xa1");
        serial_reader->flush();
        serial_reader->setTimeout(100, 100, 0, 100, 0);
        serial_reader->readline(100, "\xff\xcb\x03\xa4\xa1");
    }
    catch (std::exception e) {
        invalidDevice();
    }
    onSerialStop();
}