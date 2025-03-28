#ifndef FACE_TRACKING_H
#define FACE_TRACKING_H
#include <thread>
#include <queue>
#include <mutex>
#include <vector>
#include <inference.h>
#include <frame_types.h>
#include <kalman.h>


class Buffer;

class FaceTracking {
private:
	std::vector<std::thread*> tracking_threads;
	Buffer* buffer;
	Inference* inf_backend;

	KFTracking kf;
	std::mutex status_lock;
	TrackingStatus last_status = TRACKING_NOF;
	cv::Rect2i est_face = { 0,0,0,0 };
	std::atomic<int> next_index = 0;

	std::mutex rois_lock;
	std::vector < std::vector<cv::Point2i>> last_rois;
	float confidence = 0;
	double lost_ts=-10;
	void update_confidence(float score) {
		score -= 0.7;
		if (score > 0)
			confidence += score * kf.dt * 5 * (1 - confidence) * (1 - confidence);
		else
			confidence += score * kf.dt * 10 * confidence * confidence;
	}

	static cv::Rect2i rectify(const cv::Rect2i& dst, const cv::Size2i& size);
public:
	FaceTracking(Inference* inf_backend);
	~FaceTracking();
	void tracking_loop(int id);
	std::vector<FaceRoi*> tracking(RawFrame* frame);
};

#endif