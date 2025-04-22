#include <ImageDecoder.h>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <frame_types.h>
#include <CameraDriver.h>
#include <opencv2/imgproc.hpp>
#include <libyuv.h>

unsigned int width ;
unsigned int height ;
capture::CameraProfile* profile;
RawFrame* test_YUY2(cv::Mat frame ) {
	profile->format = PIX_TYPE_YUY2;
	unsigned char* c[3];
	c[0] = (unsigned char*)malloc(width * height * 2);
	c[1] = c[0] + (width * height);
	c[2] = c[1] + (width * height / 2);
	cv::cvtColor(frame, frame, cv::COLOR_BGR2BGRA); // Convert BGR to BGRA
	libyuv::ARGBToYUY2(frame.data, width * 4,
		c[0], width * 2,
		width, height);

	RawFrame* image = new RawFrame{
		.raw_frame = c[0],
		.raw_frame_len = unsigned int(width * height*1.5),
		.resolution = profile->resolution,
		.format = PIX_TYPE_YUY2,
		.profile = profile
	};
	image->free_funcs.push([c]() {delete c[0]; });
	return image;
}
RawFrame* test_NV12(cv::Mat frame) {
	profile->format = PIX_TYPE_NV12;
	unsigned char* c[3];
	c[0] = (unsigned char*)malloc(width * height * 1.5);
	c[1] = c[0] + (width * height);
	c[2] = c[1] + (width * height / 2);
	cv::cvtColor(frame, frame, cv::COLOR_BGR2YUV_I420);
	
	libyuv::I420ToNV12(frame.data, width,
		frame.data + width * height, width/2,
		frame.data + int(width * height * 1.25), width/2,
		c[0], width,
		c[1], width,
		width, height);

	RawFrame* image = new RawFrame{
		.raw_frame = c[0],
		.raw_frame_len = unsigned int(width * height * 3),
		.resolution = profile->resolution,
		.format = PIX_TYPE_NV12,
		.profile = profile
	};
	image->free_funcs.push([c]() {delete c[0]; });
	return image;
}

RawFrame* test_RGB24(cv::Mat frame) {
	profile->format = PIX_TYPE_BGR8;


	RawFrame* image = new RawFrame{
		.raw_frame = frame.data,
		.raw_frame_len = unsigned int(width * height * 1.5),
		.resolution = profile->resolution,
		.format = PIX_TYPE_BGR8,
		.profile = profile
	};
	image->free_funcs.push([frame]() { });
	return image;
}

int main() {
	cv::VideoCapture cap("D:\\Users\\b39b3\\Downloads\\test.mp4");
	cap.set(8, ('N', 'V', '1', '2'));
	width = 1280;
	height = 720;
	 profile = new capture::CameraProfile(nullptr);
	capture::Options* trans = new capture::CameraStream("abc", nullptr);
	trans->set_option_force(capture::STREAM_ROTATE, { 90,capture::OPTION_MANUAL });
	trans->set_option_force(capture::STREAM_FLIP_LR, { 1,capture::OPTION_MANUAL });
	trans->set_option_force(capture::STREAM_FLIP_UD, { 0,capture::OPTION_MANUAL });
	trans->set_option_force(capture::STREAM_CROP_X, { 0,capture::OPTION_MANUAL });
	trans->set_option_force(capture::STREAM_CROP_Y, { 0,capture::OPTION_MANUAL });
	trans->set_option_force(capture::STREAM_CROP_WIDTH, { (int)width / 2,capture::OPTION_MANUAL });
	trans->set_option_force(capture::STREAM_CROP_HEIGHT, { (int)height / 2,capture::OPTION_MANUAL });

	profile->resolution = { width,height};
	//profile->transform = trans;
	while (true) {
		cv::Mat frame;
		cap >> frame;
		if (frame.empty()) break;
		RawFrame * image = test_NV12(frame);
		capture::postprocess(image, trans);
		cv::Mat decoded = capture::decode_bgr(image);
		cv::imshow("Decoded Image", decoded);
		cv::waitKey(30);
		delete image;
	}
}