
#ifndef _MEDIAWRITER_H_
#define _MEDIAWRITER_H_

#include <string>
#include <opencv2/core.hpp>
#include <gwavi.h>
#include <Encoder.h>
#include <set>

class MediaWriter {
	gwavi_t* container_writer;
	encoder::stream_encoder* stream_p;
	std::thread write_file_thread;

	std::vector<double> ts_buffer;
	std::string save_filename;

	void write_file();

	std::thread* save_thread=nullptr;
	void save_signal_async();
public:
	MediaWriter(const std::string& save_path, Resolution size, Ratio fps, PIX_TYPE e_type ,PIX_TYPE d_type, int quality = -1);
	~MediaWriter();;
	void write(RawFrame* frame);
	void close();
	static std::set<PIX_TYPE> get_supported_encoders(PIX_TYPE);

};

#endif