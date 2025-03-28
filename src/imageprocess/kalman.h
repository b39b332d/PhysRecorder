#pragma once

#include <opencv2/video/tracking.hpp>
class KFTracking {
	cv::KalmanFilter KF;
	float ts;
	void set_dt(float dt);
public:
	float dt = 0.05;
	KFTracking();
	void init(const cv::Rect2i& face, float ts, float m_noise);

	void update(const cv::Rect2i& face, float ts, float m_noise);

	cv::Rect2i predict(float dt = 0.05);
};