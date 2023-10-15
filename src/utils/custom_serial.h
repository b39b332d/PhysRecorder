#ifndef CUSTOM_SERIAL_READER_H
#define CUSTOM_SERIAL_READER_H
#include <Qthread>
#include "serialport.h"
class CustomSerialReader :public QThread {
	Q_OBJECT
public:
	CustomSerialReader();
	bool setPort(const serial::PortInfo&);
	bool setPortName(const std::string& portNames);
	bool start_reading();
	bool stop_reading();
	Q_SIGNAL void serialReady(uint32_t,double);

private:
	serial::Serial serial_reader;
	int error_num = 0;
	void run();
	std::atomic_bool stop_read_sig = false;;
};
#endif