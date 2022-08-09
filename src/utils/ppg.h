#ifndef PPGREADER_H
#define PPGREADER_H
#include <Qthread>
#include "serialport.h"
class PPGReader :public QThread {
	Q_OBJECT
public:
	PPGReader();
	bool setPort(const serial::PortInfo&);
	bool setPortName(const std::string& portNames);
	bool start_reading();
	bool stop_reading();
	Q_SIGNAL void ppgReady(uint16_t,double);

private:
	serial::Serial serial_reader;
	int error_num = 0;
	void run();
	std::atomic_bool stop_read_sig = false;;
};
#endif