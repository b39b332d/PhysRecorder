#pragma once
#include <inference.h>
#include <opencv2/dnn.hpp>

#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing.h>

class InferenceDLIB :public Inference {

	dlib::frontal_face_detector* detectors;
	dlib::shape_predictor* shape_predictors;


public:
	float find_faces(FaceRoi* face, std::vector<FaceBox>& faces, const int id);
	float get_roi(FaceRoi* face, std::vector < std::vector<cv::Point2i>>& rois, const int id);

	InferenceDLIB();
	~InferenceDLIB();
};