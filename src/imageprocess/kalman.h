#pragma once

#include <opencv2/video/tracking.hpp>
class KFTracking {
	cv::KalmanFilter KF;
	float ts;
	void set_dt(float dt);
	bool is_init = false;
public:
	float dt = 0.05;
	KFTracking();
	void init(const cv::Rect2i& face, float ts, float m_noise);

	void update(const cv::Rect2i& face, float ts, float m_noise);
	void reset() {
		is_init = false; 
	}
	cv::Rect2i predict(float dt = 0.05);
};