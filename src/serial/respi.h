#ifndef RESPIREADER_H
#define RESPIREADER_H

#include <qthread.h>
#include "serialport.h"
#include "SerialReader.h"
class RESPIReader :public SerialReader {
	Q_OBJECT
public:
	RESPIReader();
	bool setPortName(const std::string& portNames);
private:
	void run();
	friend SerialReader;
};
#endif