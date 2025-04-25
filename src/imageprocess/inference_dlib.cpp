#include <inference_dlib.h>
#include <thread>

#include <dlib/opencv/cv_image.h>
#define DLIB2CV_POINT(pt) (*(cv::Point2i*)&(pt))
#define CV2DLIB_POINT(pt) (*(dlib::point*)&(pt))
float InferenceDLIB::find_faces(FaceRoi* face, std::vector<FaceBox>& faces, const int id)
{
	cv::Mat* color_mat = (cv::Mat*)(face->frame->bgr_frame);
	dlib::cv_image<dlib::bgr_pixel> img((*color_mat)(face->face));
	std::vector<dlib::rectangle> dets = detectors[id](img);
	for (auto& d : dets) {
		faces.push_back({ cv::Rect(face->face.x+d.left(),face->face.y+d.top(),d.width(),d.height()),0.9});
	}
	return 10;
}

float InferenceDLIB::get_roi(FaceRoi* face, std::vector<std::vector<cv::Point2i>>& rois, const int id)
{
	cv::Mat* color_mat = (cv::Mat*)(face->frame->bgr_frame);
	dlib::cv_image<dlib::bgr_pixel> img(*color_mat);
	auto shape = shape_predictors[id](img, dlib::rectangle(face->face.x, face->face.y, face->face.br().x, face->face.br().y));
	rois = std::vector < std::vector<cv::Point>>(2);

	rois[0] = std::vector<cv::Point2i>{
		cv::Point(shape.part(1).x(),shape.part(1).y()),
		cv::Point(shape.part(2).x(),shape.part(2).y()),
		cv::Point(shape.part(3).x(),shape.part(3).y()),
		cv::Point(shape.part(4).x(),shape.part(4).y()),
		cv::Point(shape.part(31).x(),shape.part(31).y()),
		cv::Point(shape.part(39).x(),shape.part(39).y()),
	};
	rois[1] = std::vector<cv::Point2i>{
		cv::Point(shape.part(15).x(),shape.part(15).y()),
		cv::Point(shape.part(14).x(),shape.part(14).y()),
		cv::Point(shape.part(13).x(),shape.part(13).y()),
		cv::Point(shape.part(12).x(),shape.part(12).y()),
		cv::Point(shape.part(35).x(),shape.part(35).y()),
		cv::Point(shape.part(45).x(),shape.part(45).y()),
	};
	return 1.0;
}

InferenceDLIB::InferenceDLIB()
{
	threads_n = 4;
	detectors = new dlib::frontal_face_detector[4];
	shape_predictors = new dlib::shape_predictor[4];
	auto init_func = [&](int i) {
		detectors[i] = dlib::get_frontal_face_detector();
		dlib::deserialize("data/models/dlib/shape_predictor_68_face_landmarks_GTX.dat") >> shape_predictors[i];
		};
	std::vector<std::thread> ths;
	ths.emplace_back(init_func, 0);
	ths.emplace_back(init_func, 1);
	ths.emplace_back(init_func, 2);
	ths.emplace_back(init_func, 3);
	for (auto& th : ths) {
		th.join();
	}

}

InferenceDLIB::~InferenceDLIB()
{
	delete[] detectors;
	delete[] shape_predictors;
}
