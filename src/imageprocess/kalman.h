#pragma once

#include <opencv2/video/tracking.hpp>
class KFTracking {
	cv::KalmanFilter KF;
	double ts;
	void set_dt(double dt);
	bool is_init = false;
public:
	double dt = 0.05;
	KFTracking();
	void init(const cv::Rect2i& face, double ts, float m_noise);

	void update(const cv::Rect2i& face, double ts, float m_noise);
	void reset() {
		is_init = false; 
	}
	cv::Rect2i predict(double dt = 0.05);
};