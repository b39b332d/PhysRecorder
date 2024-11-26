#include <MediaWriter.h>

void MediaWriter::write_file() {
	while (true) {
		auto jpeg_out = encoder::read(stream_p);
		if (jpeg_out == nullptr) {
			return;
		}
		else {
			gwavi_add_frame(container_writer, jpeg_out->data, jpeg_out->size);
			delete jpeg_out;
		}
	}
}
MediaWriter::MediaWriter(const std::string& save_path, Resolution size, Ratio fps, PIX_TYPE e_type, PIX_TYPE d_type, int quality) {
	stream_p = encoder::add_stream(size.width, size.height, e_type, d_type, quality);
	unsigned int a_size = 0;
	void* a_p = stream_get_info(stream_p, a_size);
	if (d_type == PIX_TYPE_RAW) {
		d_type = e_type;
	}
	container_writer = gwavi_open(save_path.c_str(), size.width, size.height, GET_PIX_TYPE_NAME(d_type).c_str() , fps.numerator,fps.denominator, NULL, a_p, a_size);
	
	write_file_thread = std::thread(&MediaWriter::write_file,this);
}
MediaWriter::~MediaWriter() {
	// close stream first
	close();
}

void MediaWriter::write(RawFrame* frame) {
	encoder::encode(frame, stream_p);
}

void MediaWriter::close() {
	if (container_writer != NULL) {
		encoder::stream_close_later(stream_p);
		write_file_thread.join();

		gwavi_close(container_writer);
		delete_stream(stream_p);
		container_writer = NULL;
	}
}