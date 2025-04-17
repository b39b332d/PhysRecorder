#ifndef FACE_TRACKING_H
#define FACE_TRACKING_H
#include <thread>
#include <queue>
#include <mutex>
#include <vector>
#include <inference.h>
#include <frame_types.h>
#include <kalman.h>

#define DEBUG_LEVEL 0

#if DEBUG_LEVEL >0
#include <iostream>
#include <stdio.h>
#include <opencv2/highgui.hpp>
#endif

#define IS_TRACKING_SUCCESS(status) (status >= 0 )
#define PRINT_DEBUG(...)\
	if (DEBUG_LEVEL > 0) { \
		printf(__VA_ARGS__); \
	} 

class Buffer;
typedef std::vector < std::vector<cv::Point2i>> RoiPoints;
class FaceTracking {
private:
	std::mutex status_lock;
	std::vector<std::thread*> tracking_threads;
	Buffer* buffer;
	Inference* inf_backend;

	KFTracking kf;
	TrackingStatus last_status = TK_DISABLE;

	RoiPoints last_rois;
	float confidence = 0;
	double lost_ts=-10;

	std::condition_variable next_index_cond;
	std::condition_variable next_index_output_cond;
	int next_index = 0;
	int next_index_output = 0;
	cv::Rect2i est_face = { 0,0,0,0 };
	bool need_reset = true;

	
	void update_confidence(float score) {
		score -= 0.7;
		if (score > 0)
			confidence += score * kf.dt * 5 * (1 - confidence) * (1 - confidence);
		else
			confidence += score * kf.dt * 10 * confidence * confidence;
	}

	static cv::Rect2i rectify(const cv::Rect2i& dst, const cv::Size2i& size);

	cv::Rect2i get_face(std::vector<FaceBox>& faces);
	cv::Scalar get_roi_color(RoiPoints& roi, cv::Mat* image);
public:
	FaceTracking(Inference* inf_backend);
	~FaceTracking();
	void tracking_loop(int id);
	void reset();
	void set_roi(cv::Rect2i);
	void tracking(RawFrame* frame);
	void disable_tracking();
	cv::Rect2i get_roi() { std::unique_lock l(status_lock); return est_face; }

	virtual void on_face_lost();
	virtual void on_signal_ready(FaceRoi* signal);
};

#endif