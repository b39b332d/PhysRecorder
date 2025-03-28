#pragma once
#include <inference.h>
#include <opencv2/dnn.hpp>

class InferenceOV :public Inference {


	std::vector<cv::dnn::Net> nets_face;
	std::vector<cv::dnn::Net> nets_landmarks;

	static cv::Rect2i getSquareBox(cv::Rect2i& face, cv::Size frame_size, double scale = 1.2);

public:
	float find_faces(FaceRoi* face, std::vector<FaceBox>& faces, const int id);
	float get_roi(FaceRoi* face, std::vector < std::vector<cv::Point2i>>& rois, const int id);

	InferenceOV();
};