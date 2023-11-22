
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
	MediaWriter(const std::string& save_path, cv::Size2i size, double fps, PIX_TYPE e_type = PIX_TYPE_BGR888,float quality = -1);
	~MediaWriter();;
	void write(const cv::Mat& frame);
	void close();

};

#endif