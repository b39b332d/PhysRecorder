#ifndef PPGREADER_H
#define PPGREADER_H
#include <Qthread>
#include "serialport.h"
#include "SerialReader.h"
class PPGReader :public SerialReader {
	Q_OBJECT
public:
	PPGReader();
	bool setPortName(const std::string& portNames);

private:
	void run();
	friend SerialReader;
};
#endif