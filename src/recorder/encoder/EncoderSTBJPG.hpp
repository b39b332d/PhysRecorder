#pragma once
#define UNICODE
#include <vector>
#include <iostream>
#include <EncoderComp.h>

#include "stb_image_write.h"

namespace encoder {

	class EncoderSTBJPG :public EncoderComp {
		std::atomic<int> jpg_size_pred;
		int jpg_size_pred_max;
	public:
		EncoderSTBJPG(int width, int height, PIX_TYPE e_type, int quality);
		EncodedFrame* encode(RawFrame* rgbData);
		~EncoderSTBJPG();
	};

	EncoderSTBJPG::EncoderSTBJPG(int width, int height, PIX_TYPE e_type, int quality) : EncoderComp(width, height, e_type, quality) {
		jpg_size_pred = width * height * comp * 0.1;
		jpg_size_pred_max = width * height * comp;
		if (quality <0)quality = 90; // set default 90; range[1,100]
		if (quality > 100)quality = 100;
	}
	struct EncoderSTBJPG_ctx {
		unsigned char* img;
		int img_size;
		int current_p;
		EncoderSTBJPG* ctx;
	};
	EncodedFrame* EncoderSTBJPG::encode(RawFrame* rgbData)
	{
		auto img = (unsigned char*)malloc(jpg_size_pred);
		auto temp =  EncoderSTBJPG_ctx{ img ,jpg_size_pred,0, this};
		auto ibda = [](void* cont, void* data, int dataLength) {
			EncoderSTBJPG_ctx* t = (EncoderSTBJPG_ctx*)cont;
			if (t->current_p + dataLength > t->img_size) {
				size_t newCapacity = t->img_size * 2;
				newCapacity = newCapacity > t->ctx->jpg_size_pred_max ? t->ctx->jpg_size_pred_max : newCapacity;
				auto new_buf = (unsigned char*)malloc(newCapacity);
				std::memcpy(new_buf, t->img, t->current_p);
				delete[] t->img;
				t->img = new_buf;
				t->img_size = newCapacity;
			}
			std::memcpy(t->img + t->current_p, data, dataLength);
			t->current_p += dataLength;
			};
		stbi_write_jpg_to_func(ibda,&temp, width, height, comp, rgbData.get(), 90);
		return new EncodedFrame( temp.img, temp.current_p );
	}

	EncoderSTBJPG::~EncoderSTBJPG() {
	}
};
