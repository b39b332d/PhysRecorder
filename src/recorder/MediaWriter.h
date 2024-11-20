
#ifndef _MEDIAWRITER_H_
#define _MEDIAWRITER_H_

#include <string>
#include <opencv2/core.hpp>
#include <gwavi.h>
#include <Encoder.h>

class MediaWriter {
	gwavi_t* container_writer;
	encoder::stream_encoder* stream_p;
	std::thread write_file_thread;

	void write_file();

public:
	MediaWriter(const std::string& save_path, Resolution size, Ratio fps, PIX_TYPE e_type ,PIX_TYPE d_type, int quality = -1);
	~MediaWriter();;
	void write(RawFrame* frame);
	void close();

};

#endif