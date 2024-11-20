#pragma once
#define UNICODE
#include <EncoderComp.h>


namespace encoder {

	class EncoderRaw :public EncoderComp {
		int size_max;
		int line_width;
	public:
		EncoderRaw(int width, int height, PIX_TYPE e_type, int quality);
		EncodedFrame* encode(RawFrame* rgbData);
		~EncoderRaw();
	};

	EncoderRaw::EncoderRaw(int width, int height, PIX_TYPE e_type, int quality) : EncoderComp(width, height, e_type, quality) {
		size_max = width * comp * height ;
		line_width = width * comp;
	}
	EncodedFrame* EncoderRaw::encode(RawFrame* rgbData)
	{
		unsigned char*  temp_buf =  (unsigned char*)malloc(size_max);
		for (int i = 0; i < height; i++) {
			void* s1 = rgbData->raw_frame + line_width * i, *s2= temp_buf + line_width * (height - i-1);
			memcpy(s2, s1, line_width);
		}
		return new EncodedFrame(temp_buf, size_max);
	}

	EncoderRaw::~EncoderRaw() {
	}
};
