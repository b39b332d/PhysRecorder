#ifndef _ENCODER_H_
#define _ENCODER_H_
#include <vector>
#include <memory>
#include <frame_types.h>
#include <utility>
#include <set>

namespace encoder {

	class stream_encoder;

	void encoder_init();
	void encode(RawFrame* frame,  stream_encoder* stream_p);
	stream_encoder* add_stream(int width, int height, PIX_TYPE e_type, PIX_TYPE d_type, int quality = -1);
	void *stream_get_info(stream_encoder*,unsigned int&);
	void stream_close_later(stream_encoder* stream_enc);
	EncodedFrame* read(stream_encoder* stream_p);
	void encoder_stop();
	void delete_stream(stream_encoder* stream_p);


	std::set<PIX_TYPE> get_supported_encoders(PIX_TYPE);
};
#endif
