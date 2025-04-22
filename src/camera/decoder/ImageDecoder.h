#pragma once
#include <opencv2/imgcodecs.hpp>
class RawFrame;
namespace capture {
	class Options;
	void postprocess(RawFrame* image, Options* transform);
	cv::Mat decode_bgr(RawFrame* image);
	cv::Mat decode_yuv(RawFrame* image);

}