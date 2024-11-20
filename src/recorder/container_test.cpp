#include <MediaWriter.h>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <chrono>
#include <iostream>
#include <windows.h>
#include <libyuv.h>
#include <opencv2/videoio.hpp>
int main() {

	cv::Mat img = cv::imread("data/resources/face.jpg");
	cv::resize(img, img, cv::Size(1280, 720));
	cv::cvtColor(img, img, cv::COLOR_BGR2RGBA);
	unsigned char* tt = (unsigned char*)malloc(img.cols * img.rows * 4);

	libyuv::RGBAToARGB(img.data, img.cols * 4, tt, img.cols * 4, img.cols, img.rows);
	auto cap = cv::VideoCapture("D:/out.avi");
	cv::Mat a;
	cap.read(a);

	MediaWriter mw("D:/out.avi", Resolution{ unsigned(img.cols),unsigned(img.rows) }, Ratio{ 30, 1 }, PIX_TYPE_NV12, PIX_TYPE_MJPG, -1);

	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	for (int i = 0; i < 30; i++) {
		unsigned char* temp = (unsigned char*)malloc(img.cols * img.rows * 3);
		libyuv::ARGBToNV12(tt, img.cols * 4,
			temp, img.cols, temp+ img.cols * img.rows, img.cols, img.cols, img.rows);
		cv::Mat out;
		cv::cvtColor(cv::Mat(img.rows*1.5, img.cols, CV_8UC1, temp), out, cv::COLOR_YUV2BGR_NV12); 


		
		RawFrame* frame = new RawFrame;
		frame->raw_frame = temp;
		frame->free_funcs.push([temp]() {
			free(temp);
			});
		mw.write(frame);
	}
	mw.close();
	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	std::cout << "Time difference = " << (double)std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000000 << "[s]" << std::endl;

	return 0;
}