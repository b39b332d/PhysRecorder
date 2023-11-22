#include <MediaWriter.h>

void MediaWriter::write_file() {
	while (true) {
		auto jpeg_out = encoder::read(stream_p);
		if (jpeg_out.size() == 0) {
			return;
		}
		else {
			gwavi_add_frame(container_writer, jpeg_out.data(), jpeg_out.size());
		}
	}
}
MediaWriter::MediaWriter(const std::string& save_path, cv::Size2i size, double fps, PIX_TYPE e_type, float quality) {
	container_writer = gwavi_open(save_path.c_str(), size.width, size.height, "MJPG", fps, NULL);
	stream_p = encoder::add_stream(size.width, size.height, e_type, quality);
	write_file_thread = std::thread(&MediaWriter::write_file,this);
}
MediaWriter::~MediaWriter() {
	// close stream first
	close();
}

void MediaWriter::write(const cv::Mat& frame) {
	encoder::encode(std::shared_ptr<void>(frame.data,[frame](auto p){}), stream_p);
}

void MediaWriter::close() {
	if (container_writer != NULL) {
		encoder::stream_close_later(stream_p);
		write_file_thread.join();

		gwavi_close(container_writer);
		container_writer = NULL;
	}
}