#pragma once
#define UNICODE
#include <vector>
#include <iostream>
#include <EncoderComp.h>
#include<huffyuv.hpp>
#include<libyuv.h>

namespace encoder {
	class EncoderHUFF :public EncoderComp {
		::huffyuv* encoder;
		PIX_TYPE format;
	public:
		EncoderHUFF(int width, int height, PIX_TYPE e_type, int quality);
		EncodedFrame* encode(RawFrame* rgbData);
		~EncoderHUFF();
	};
	EncoderHUFF::EncoderHUFF(int width, int height, PIX_TYPE e_type, int quality): EncoderComp(width, height, e_type, quality){//huffyuv °æ±¾
		format = e_type;
		if (format == PIX_TYPE_YUY2 || format == PIX_TYPE_UYVY || format == PIX_TYPE_YUYV || format == PIX_TYPE_I422 ) {
			encoder = new ::huffyuv(width, height, height > ::huffyuv::interlaced_threshold, false, ::huffyuv::format_type::yuyv, ::huffyuv::predictor_type::left);
			encoder->is_uyvy = format == PIX_TYPE_UYVY;
			data_size = width * height * 2;
		}
		else if(format == PIX_TYPE_I420 || format == PIX_TYPE_NV12 || format == PIX_TYPE_NV21 || format == PIX_TYPE_Y12I){
			encoder = new ::huffyuv(width, height, height > ::huffyuv::interlaced_threshold, true, ::huffyuv::format_type::yuyv, ::huffyuv::predictor_type::left);

			data_size = width * height * 1.5;
		}
		else if (format == PIX_TYPE_RGB8 || format == PIX_TYPE_BGR8 || format == PIX_TYPE_L8) {
			encoder = new ::huffyuv(width, height, height > ::huffyuv::interlaced_threshold, true, ::huffyuv::format_type::bgr, ::huffyuv::predictor_type::left);

			data_size = width * height * 3;
		}
		else {
			encoder = new ::huffyuv(width, height, height > ::huffyuv::interlaced_threshold, true, ::huffyuv::format_type::bgra, ::huffyuv::predictor_type::left);

			data_size = width * height * 4;
		}
		additional_data = encoder->generate_stream_header(additional_data_size);
		
	}
	EncodedFrame* EncoderHUFF::encode(RawFrame* rgbData) {
		unsigned char* c[5];
		unsigned char* encoded_img = (unsigned char*)malloc(data_size);
		unsigned long long int img_size = data_size;
		if (format == PIX_TYPE_YUY2 || format == PIX_TYPE_YUYV || format == PIX_TYPE_UYVY) {
			encoder->encode(rgbData->raw_frame, rgbData->raw_frame_len, encoded_img, img_size);
			rgbData->release();
		}
		else if (format == PIX_TYPE_I422) {
			c[3] = (unsigned char*)malloc(width * height * 2);
			libyuv::I422ToYUY2(rgbData->raw_frame, width,
				rgbData->raw_frame+width*height, width / 2,
				rgbData->raw_frame + int(width * height*1.5), width / 2,
				c[3], width * 2,
				width, height);
			rgbData->release();
			encoder->encode(c[3], width * height * 2, encoded_img, img_size);

			free(c[3]);
		}
		else if (format == PIX_TYPE_NV12 || format == PIX_TYPE_NV21 ) {
			c[0] = (unsigned char*)malloc(width * height);
			c[1] = (unsigned char*)malloc(width * height / 4);
			c[2] = (unsigned char*)malloc(width * height / 4);
			c[3] = (unsigned char*)malloc(width * height * 2);
			if(format == PIX_TYPE_NV12)
			libyuv::NV12ToI420(rgbData->raw_frame, width,
				rgbData->raw_frame + width * height, width,
				c[0], width, c[1], width / 2, c[2], width / 2,
				width, height);
			else if(format == PIX_TYPE_NV12)
				libyuv::NV21ToI420(rgbData->raw_frame, width,
					rgbData->raw_frame + width * height, width,
					c[0], width, c[1], width / 2, c[2], width / 2,
					width, height);
			rgbData->release();
			libyuv::I420ToYUY2(c[0], width, c[1], width / 2, c[2], width / 2,
				c[3], width * 2, width, height);
			encoder->encode(c[3], width * height * 2, encoded_img, img_size);

			free(c[0]);
			free(c[1]);
			free(c[2]);
			free(c[3]);
		}
		else if (format == PIX_TYPE_Y12I || format == PIX_TYPE_I420) {
			c[3] = (unsigned char*)malloc(width * height * 2);
			libyuv::I420ToYUY2(rgbData->raw_frame, width, rgbData->raw_frame+width*height, width / 2, rgbData->raw_frame+int(width*height*1.25), width / 2,
				c[3], width * 2, width, height);
			rgbData->release();
			encoder->encode(c[3], width * height * 2, encoded_img, img_size);
			free(c[3]);
		}
		else if (format == PIX_TYPE_BGR8) {
			encoder->encode(rgbData->raw_frame, width * height * 3, encoded_img, img_size);
			rgbData->release();
		}
		else if (format == PIX_TYPE_RGB8) {
			encoder->encode(rgbData->raw_frame, width * height * 3, encoded_img, img_size);
			rgbData->release();
		}
		return new EncodedFrame(encoded_img, (int)img_size);
	}

	EncoderHUFF::~EncoderHUFF() {
		delete encoder;
		if (additional_data) free(additional_data);
	}
};
