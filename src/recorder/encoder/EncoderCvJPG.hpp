#pragma once
#include <vector>
#include <iostream>
#include <EncoderComp.h>
#include <opencv2/imgcodecs.hpp>

namespace encoder {
	class EncoderCvJPG: public EncoderComp {
		bool subsample = true;
	public:
		EncoderCvJPG(int width, int height, PIX_TYPE e_type, int quality);
		EncodedFrame* encode(RawFrame* rgbData);
		~EncoderCvJPG();
		static bool is_support(PIX_TYPE e_type) {
			return true;
		}
	};
	EncoderCvJPG::EncoderCvJPG(int width, int height, PIX_TYPE e_type, int quality): EncoderComp(width, height, e_type, quality){

		// quality default -1, range [-1 ,0~100,101]
		// WIC map to	   95        [95 ,0~100]
		if (quality > 100) {
			quality = 100;
			subsample = false;
		}
		else if (quality < 0) {
			quality = -1;
		}

	}
	EncodedFrame* EncoderCvJPG::encode(RawFrame* rgbData) {
		std::vector<uchar>* buf = new std::vector<uchar>;
		cv::Mat img(height, width, CV_MAKETYPE(CV_8U, comp), (uint8_t*)rgbData->bgr_frame);
		if(quality == -1)
			cv::imencode(".jpg", img, *buf);
		else
			if(subsample)
				cv::imencode(".jpg", img, *buf, { cv::IMWRITE_JPEG_QUALITY,quality});
			else
				cv::imencode(".jpg", img, *buf, { cv::IMWRITE_JPEG_QUALITY,quality,cv::IMWRITE_JPEG_SAMPLING_FACTOR,cv::IMWRITE_JPEG_SAMPLING_FACTOR_444});
		
		return new EncodedFrame(buf->data(), (unsigned int)buf->size(), [buf]() {delete (std::vector<uchar>*)buf; });
	}

	EncoderCvJPG::~EncoderCvJPG() {
	}
};
