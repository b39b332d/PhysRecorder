#ifndef CUSTOM_SERIAL_READER_H
#define CUSTOM_SERIAL_READER_H
#include <Qthread>
#include "serialport.h"
#include "SerialReader.h"
class CustomSerialReader :public SerialReader {
	Q_OBJECT
public:
	CustomSerialReader();
	bool setPortName(const std::string& portNames);

private:
	void run();
	friend SerialReader;
};
#endif