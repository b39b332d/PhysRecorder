
#include <face_tracking.h>
#include <inference_openvino.h>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
int main() {
	InferenceOV ov_inf;
	FaceTracking tracker(&ov_inf);

	//cv::VideoCapture cap("//A406-server/nbslab-public/数据集/ECG-Fitness/视频版/00/01/c920-1.avi");
	cv::VideoCapture cap(1);
	cv::Mat frame;
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
}