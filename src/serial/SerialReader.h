#pragma once

#include <Qthread>
#include <vector>
#include <string>
#include <mutex>
#include <thread>
namespace serial {
	class PortInfo;
	class Serial;
}

class QCPGraph;
class SerialReader:public QObject {
	Q_OBJECT
public:
	SerialReader(unsigned int baudrate = 9600,const std::string& n="");
	virtual ~SerialReader();
	bool setPort(const serial::PortInfo&);
	virtual bool setPortName(const std::string& portNames)=0;
	bool start_reading();
	bool stop_reading();
	void serial_add_signal(double , double);
	void invalidDevice();
	std::string device_name;//show in comboBox
	std::string friendly_name; // save and show file name
	std::string signal_name; // set by child class
	Q_SIGNAL void serial_ready(double, double);
	Q_SIGNAL void serial_stopped(SerialReader*);

	bool is_invalid = false;
	bool is_running = false;
	std::thread* th = nullptr;

	QCPGraph* graph=nullptr;

	std::vector<double> signal_buffer;
	std::vector<double> ts_buffer;
	std::string save_filename;
	std::mutex serial_lock;
	std::thread *save_thread = nullptr;
	bool is_recording = false;
	void startRecording(std::string filename);
	void stopRecording();

	static std::map<std::string, SerialReader*> serial_readers;
	static void refresh_serials();
protected:
	serial::Serial* serial_reader;
	std::atomic_bool stop_read_sig = false;;
	void onSerialStop();

private:
	double last_ts = 0;
	void save_signal_async();
	void run_wrapper() {
		run();
	}
	virtual void run() = 0;
};