#include <kalman.h>

KFTracking::KFTracking():KF(6, 4) {
	cv::setIdentity(KF.transitionMatrix);
	cv::setIdentity(KF.measurementMatrix);
	cv::setIdentity(KF.processNoiseCov);
	cv::setIdentity(KF.measurementNoiseCov);
	cv::setIdentity(KF.errorCovPost);
}

void KFTracking::set_dt(float dt) {
	float dt_2 = dt * dt;
	float dt_3 = dt_2 * dt;
	float dt_4 = dt_3 * dt;
	dt_3 /= 2;
	dt_4 /= 4;
	KF.processNoiseCov.at<float>(0, 0) = dt_4;
	KF.processNoiseCov.at<float>(1, 1) = dt_4;
	KF.processNoiseCov.at<float>(2, 2) = dt_4;
	KF.processNoiseCov.at<float>(3, 3) = dt_4;
	KF.processNoiseCov.at<float>(4, 4) = dt_2;
	KF.processNoiseCov.at<float>(5, 5) = dt_2;
	KF.processNoiseCov.at<float>(0, 4) = dt_3;
	KF.processNoiseCov.at<float>(1, 5) = dt_3;
	KF.processNoiseCov.at<float>(4, 0) = dt_3;
	KF.processNoiseCov.at<float>(5, 1) = dt_3;
	KF.processNoiseCov *= 1000; // trust little about this model, so we set a large process noise
	KF.transitionMatrix.at<float>(0, 4) = dt;
	KF.transitionMatrix.at<float>(1, 5) = dt;
}


void KFTracking::init(const cv::Rect2i& face, float ts, float m_noise) {
	m_noise *= m_noise;
	KF.statePost.at<float>(0) = (float)face.x + (float)face.width / 2;
	KF.statePost.at<float>(1) = (float)face.y + (float)face.height / 2;
	KF.statePost.at<float>(2) = face.width;
	KF.statePost.at<float>(3) = face.height;
	KF.statePost.at<float>(4) = 0;
	KF.statePost.at<float>(5) = 0;

	cv::setIdentity(KF.errorCovPost);
	KF.errorCovPost *= m_noise;
	this->ts = ts;
	is_init = true;
}
void KFTracking::update(const cv::Rect2i& face, float ts, float m_noise) {
	if (!is_init) {
		init(face, ts, m_noise);
		return;
	}
	dt = ts - this->ts;
	this->ts = ts;
	set_dt(dt);
	cv::Mat measurement(4, 1, CV_32F);
	measurement.at<float>(0) = (float)face.x + (float)face.width / 2;
	measurement.at<float>(1) = (float)face.y + (float)face.height / 2;
	measurement.at<float>(2) = face.width;
	measurement.at<float>(3) = face.height;

	KF.measurementNoiseCov.at<float>(0, 0) = m_noise * m_noise;
	KF.measurementNoiseCov.at<float>(1, 1) = m_noise * m_noise;
	KF.measurementNoiseCov.at<float>(2, 2) = (m_noise / 2) * (m_noise / 2);
	KF.measurementNoiseCov.at<float>(3, 3) = (m_noise / 2) * (m_noise / 2);
	KF.predict();
	KF.correct(measurement);
}
cv::Rect2i KFTracking::predict(float dt) {
	set_dt(dt);
	cv::Mat prediction = KF.transitionMatrix * KF.statePost;
	float width = prediction.at<float>(2);
	float height = prediction.at<float>(3);
	return cv::Rect2i((int)(prediction.at<float>(0) - width / 2), (int)(prediction.at<float>(1) - height / 2), (int)width, (int)height);
}
