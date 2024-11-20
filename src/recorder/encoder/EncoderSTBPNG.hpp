#pragma once
#define UNICODE
#include <vector>
#include <iostream>
#include <EncoderComp.h>
#include <span>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
namespace encoder {

	class EncoderSTBPNG :public EncoderComp {
	public:
		EncoderSTBPNG(int width, int height, PIX_TYPE e_type, int quality);
		EncodedFrame encode(RawFrame* rgbData);
		~EncoderSTBPNG();
	};
	EncoderSTBPNG::EncoderSTBPNG(int width, int height, PIX_TYPE e_type, int quality) : EncoderComp(width, height, e_type, quality) {
		if (quality <0 ) quality = 8; // compress level [0-9] default 8
		else if (quality > 9) quality = 9;
	}
	EncodedFrame* EncoderSTBPNG::encode(RawFrame* rgbData)
	{
		int out_len;
		unsigned char* temp = stbi_write_png_to_mem(rgbData.get(), 0, width, height, comp, &out_len, quality);
		EncodedFrame* frame = new EncodedFrame( temp, out_len );
		return frame;
	}

	EncoderSTBPNG::~EncoderSTBPNG() {
	}
};
