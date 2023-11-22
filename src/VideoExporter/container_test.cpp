#include <MediaWriter.h>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <chrono>
#include <iostream>

int main() {
	cv::Mat img = cv::imread("data/resources/face.jpg");
	cv::resize(img, img, cv::Size(1280 * 2, 720));


	MediaWriter mw("D:/out.avi", img.size(),10);

	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	for (int i = 0; i < 1200; i++) {
		mw.write(img);
	}
	mw.close();
	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	std::cout << "Time difference = " << (double)std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000000 << "[s]" << std::endl;

	return 0;
}