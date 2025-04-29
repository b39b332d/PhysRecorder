#ifndef PPGREADER_H
#define PPGREADER_H
#include "SerialReader.h"
class PPGReader :public SerialReader {
public:
	PPGReader();
	bool setPortName(const std::string& portNames) override;
	char device_c;
private:
	void run() override;
	friend SerialReader;
};
#endif