#ifndef RESPIREADER_H
#define RESPIREADER_H

#include <qthread.h>
#include "SerialReader.h"
class RESPIReader :public SerialReader {
public:
	RESPIReader();
	bool setPortName(const std::string& portNames) override;

private:
	void run() override;
	friend SerialReader;
};
#endif