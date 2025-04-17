#pragma once
#include <opencv2/core.hpp>
#include <frame_types.h>

typedef enum {
	TK_DISABLE = -4,
	TK_STATIC = -3,
	TK_NOFACE = -2,
	TK_LOSS = -1,
	TK_UNFINISH = 0,
	TK_FINISH = 1,
	TK_FIRST = 2,
} TrackingStatus;

struct FaceRoi
{
	double ts;
	double r, g, b;
	cv::Rect2i face;
	RawFrame* frame;
	int index;
	int session;
	TrackingStatus status;
};
typedef enum {
	INF_OPENVINO = 0,
	INF_DLIB = 1,
} InferenceType;

typedef  std::pair<cv::Rect2i, float> FaceBox;

class Inference {
public:
	int threads_n;
	virtual float find_faces(FaceRoi* face, std::vector<FaceBox>& faces, const int id) = 0;
	virtual float get_roi(FaceRoi* face, std::vector < std::vector<cv::Point2i>>& rois, const int id) = 0;
	static Inference* get_inference(InferenceType inf_type);
};