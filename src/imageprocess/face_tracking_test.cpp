
#include <face_tracking.h>
#include <inference_openvino.h>
#include <inference_dlib.h>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
int main() {
	InferenceDLIB ov_inf;
	//InferenceOV ov_inf;
	FaceTracking tracker(&ov_inf);

	//cv::VideoCapture cap("//A406-server/nbslab-public/数据集/ECG-Fitness/视频版/00/01/c920-1.avi");

	std::thread a([&tracker]() {
		cv::VideoCapture cap(1);
		cv::Mat frame;
		tracker.set_roi({ 0,0,0,0 });
		while (1) {
			cap >> frame;
			if (frame.empty()) {
				break;
			}
			RawFrame* raw_frame = new RawFrame;
			raw_frame->bgr_frame = new cv::Mat(frame);
			raw_frame->frame_ts = cap.get(cv::CAP_PROP_POS_MSEC) * 1e3;
			raw_frame->free_funcs.push([raw_frame]() {
				delete raw_frame->bgr_frame;
				});
			tracker.tracking(raw_frame);
		}
		});
	//std::thread([&tracker]() {
	//	for (int i = 0; i < 5;i++) {
	//		_sleep(5000);
	//		tracker.reset();
	//	}
	//	_sleep(5000);
	//	tracker.set_roi({ 0,0,100,100 });
	//	}).detach();
	a.join();
}