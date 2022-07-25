#ifndef RESPIREADER_H
#define RESPIREADER_H

#include <qthread.h>
#include "serialport.h"
class RESPIReader :public QThread {
	Q_OBJECT
public:
	RESPIReader();
	bool setPortName(const QString& portNames);
	bool start_reading();
	bool stop_reading();
	Q_SIGNAL void respiReady(uchar,double);
private:
	serial::Serial serial_reader;
	int error_num = 0;
	void run();
	std::atomic_bool stop_read_sig = false;;
};
#endif