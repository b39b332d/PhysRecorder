#pragma once
#define UNICODE
#include <vector>
#include <iostream>
#include <EncoderComp.h>
#include <opencv2/imgcodecs.hpp>

namespace encoder {
	class EncoderCvPNG :public EncoderComp {
	public:
		EncoderCvPNG(int width, int height, PIX_TYPE e_type, int quality);
		EncodedFrame* encode(RawFrame* rgbData);
		~EncoderCvPNG();
	};
	EncoderCvPNG::EncoderCvPNG(int width, int height, PIX_TYPE e_type, int quality): EncoderComp(width, height, e_type, quality){
		if (quality < 0) quality = -1; // compress level [0-9] default 1
		else if (quality >9) quality = 9;
	}
	EncodedFrame* EncoderCvPNG::encode(RawFrame* rgbData) {
		std::vector<uchar>* buf = new std::vector<uchar>;
		cv::Mat img(height, width, CV_MAKETYPE(CV_8U,comp),(uint8_t*) rgbData->raw_frame);
		if(quality == -1)
			cv::imencode(".png", img, *buf);
		else
			cv::imencode(".png", img, *buf, {cv::IMWRITE_PNG_COMPRESSION ,quality});
		return new EncodedFrame(buf->data(), buf->size(), [buf]() {delete buf; });
	}

	EncoderCvPNG::~EncoderCvPNG() {
	}
};
