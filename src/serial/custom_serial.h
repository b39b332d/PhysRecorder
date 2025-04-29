#ifndef CUSTOM_SERIAL_READER_H
#define CUSTOM_SERIAL_READER_H
#include "SerialReader.h"
class CustomSerialReader :public SerialReader {
public:
	CustomSerialReader();
	bool setPortName(const std::string& portNames) override;

private:
	void run() override;
	friend SerialReader;
};
#endif