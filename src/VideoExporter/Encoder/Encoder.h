#ifndef _ENCODER_H_
#define _ENCODER_H_
#include <vector>
#include <memory>
#include <CameraDriver.h>

namespace encoder {

	class stream_encoder;

	void encoder_init();
	void encode(std::shared_ptr<void> frame,  stream_encoder* stream_p);
	stream_encoder* add_stream(int width, int height, PIX_TYPE e_type= PIX_TYPE_RGB888, float quality=-1);
	void stream_close_later(stream_encoder* stream_enc);
	std::vector<unsigned char> read(stream_encoder* stream_p);
	void encoder_stop();
};
#endif
