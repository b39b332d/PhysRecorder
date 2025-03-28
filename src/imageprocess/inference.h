#pragma once
#include <opencv2/core.hpp>
#include <frame_types.h>

typedef enum {
	TRACKING_NOF = -2,
	TRACKING_LOS = -1,
	TRACKING_UNF = 0,
	TRACKING_FIN = 1,
	TRACKING_FST = 2
} TrackingStatus;
typedef  std::pair<cv::Rect2i, float> FaceBox;
struct FaceRoi
{
	double ts;
	double r, g, b;
	cv::Rect2i face;
	RawFrame* frame;
	int index;
	TrackingStatus status;
};
class Inference {
public:
	int threads_n;
	virtual float find_faces(FaceRoi* face, std::vector<FaceBox>& faces, const int id) = 0;
	virtual float get_roi(FaceRoi* face, std::vector < std::vector<cv::Point2i>>& rois, const int id) = 0;
};