#include <inference_openvino.h>
#include <opencv2/imgcodecs.hpp>
cv::Rect2i InferenceOV::getSquareBox(cv::Rect2i& face, cv::Size frame_size, double scale) {
	float max_size = std::max(face.width, face.height) * scale;
	float center_x = face.x + face.width / 2;
	float center_y = face.y + face.height / 2;
	if (center_x - max_size / 2 < 0)
		max_size = 2 * center_x;
	if (center_y - max_size / 2 < 0)
		max_size = 2 * center_y;
	if (center_y + max_size / 2 > frame_size.height)
		max_size = 2 * (frame_size.height - center_y);
	if (center_x + max_size / 2 > frame_size.width)
		max_size = 2 * (frame_size.width - center_x);
	return cv::Rect2i(center_x - max_size / 2,
		center_y - max_size / 2, max_size, max_size);
}

float InferenceOV::find_faces(FaceRoi* face, std::vector<FaceBox>& faces, const int id) {
	cv::Mat* color_mat = (cv::Mat*)(face->frame->bgr_frame);
	nets_face[id].setInput(cv::dnn::blobFromImage((*color_mat)(face->face), 1, cv::Size(300, 300)));
	auto face_ofs = face->face.tl();
	auto face_scale = face->face.size();
	cv::Mat prob = nets_face[id].forward();
	cv::Mat preds_face(prob.size[2], prob.size[3], CV_32F, prob.ptr<float>());
	for (uchar row = 0; row < preds_face.size[0]; row++) {
		const float* pred = preds_face.ptr<float>(row);
		if (pred[2] > 0.8) {
			cv::Rect2i face_rect(cv::Point2i(pred[3] * face_scale.width + face_ofs.x, pred[4] * face_scale.height + face_ofs.y), cv::Point2i(pred[5] * face_scale.width + face_ofs.x, pred[6] * face_scale.height + face_ofs.y));
			if (face_rect.area() > 0)
				faces.push_back({ face_rect, pred[2] });
		}
		else break;
	}
	return  (float)std::max(color_mat->cols, color_mat->rows) / 300;
}

float InferenceOV::get_roi(FaceRoi* face, std::vector < std::vector<cv::Point2i>>& rois, const int id) {
	cv::Mat* color_mat = (cv::Mat*)(face->frame->bgr_frame);
	auto face_region = getSquareBox(face->face, color_mat->size(), 1.2);
	nets_landmarks[id].setInput(cv::dnn::blobFromImage((*color_mat)(face_region), 1, cv::Size(60, 60)));
	cv::Mat raw_preds_landmarks = nets_landmarks[id].forward();
	raw_preds_landmarks = raw_preds_landmarks * face_region.width;
	const float* pred = raw_preds_landmarks.ptr<float>(0);

	cv::Point face_left_top(pred[38], pred[39]);
	cv::Point face_right_top(pred[66], pred[67]);
	if (pred[0] < 0.00001 && pred[1] < 0.0001 || face_left_top.x - face_right_top.x >= 0) {
		return -1;
	}
	float k = (face_left_top.y - face_right_top.y) / (face_left_top.x - face_right_top.x);
	float b = face_left_top.y - k * face_left_top.x;
	cv::Point face_6(pred[12], pred[13]), face_7(pred[14], pred[15]);
	float x = (face_6.x + (face_6.y - b) * k) / (1 + k * k);
	cv::Point face_center_left(x, k * x + b);
	x = (face_7.x + (face_7.y - b) * k) / (1 + k * k);
	cv::Point face_center_right(x, k * x + b);


	rois = std::vector < std::vector<cv::Point>>(2);
	auto face_ofs = face_region.tl();
	rois[0] = std::vector<cv::Point>{ face_6 + face_ofs, face_center_left + face_ofs, cv::Point((pred[40] + pred[38]) / 2, (pred[41] + pred[39]) / 2) + face_ofs,
		cv::Point(pred[40], pred[41]) + face_ofs, cv::Point(pred[42], pred[43]) + face_ofs,
		cv::Point(pred[44], pred[45]) + face_ofs, cv::Point(pred[46], pred[47]) + face_ofs,
		cv::Point(pred[16], pred[17]) + face_ofs };
	rois[1] = std::vector<cv::Point>{ cv::Point((pred[66] + pred[64]) / 2, (pred[67] + pred[65]) / 2) + face_ofs, face_center_right + face_ofs, face_7 + face_ofs,
		cv::Point(pred[18], pred[19]) + face_ofs, cv::Point(pred[58], pred[59]) + face_ofs,
		cv::Point(pred[60], pred[61]) + face_ofs, cv::Point(pred[62], pred[63]) + face_ofs,
		cv::Point(pred[64], pred[65]) + face_ofs };
	return  (float)face_region.width / 60;
}


InferenceOV::InferenceOV() {
	std::string root_path = "./data/";
	std::string path_net_facedetect = root_path + "models/intel/face-detection-retail-0005/FP16/face-detection-retail-0005";
	std::string path_net_landmarks = root_path + "models/intel/facial-landmarks-35-adas-0002/FP16/facial-landmarks-35-adas-0002";
	std::string resource_path = root_path + "resources/";
	nets_face = std::vector<cv::dnn::Net>(2);
	nets_landmarks = std::vector<cv::dnn::Net>(2);
	threads_n = 2;
	cv::Mat test_faces = cv::imread(resource_path + "faces.jpg");
	cv::Mat test_face = cv::imread(resource_path + "face.jpg");
	auto face_func = [&](int i) {
		auto net_face = cv::dnn::readNetFromModelOptimizer(path_net_facedetect + ".xml", path_net_facedetect + ".bin");
		net_face.setPreferableBackend(cv::dnn::DNN_BACKEND_INFERENCE_ENGINE);
		net_face.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
		net_face.setInput(cv::dnn::blobFromImage(test_faces, 1, cv::Size(300, 300)));
		net_face.forward();
		nets_face[i] = net_face;
		};
	auto lmdk_func = [&](int i) {
		auto net_landmarks = cv::dnn::readNetFromModelOptimizer(path_net_landmarks + ".xml", path_net_landmarks + ".bin");
		net_landmarks.setPreferableBackend(cv::dnn::DNN_BACKEND_INFERENCE_ENGINE);
		net_landmarks.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
		net_landmarks.setInput(cv::dnn::blobFromImage(test_face, 1, cv::Size(60, 60)));
		net_landmarks.forward();
		nets_landmarks[i] = net_landmarks;
		};
	std::vector<std::thread> ths;
	ths.emplace_back( face_func,0);
	ths.emplace_back(face_func, 1);
	ths.emplace_back(lmdk_func, 0);
	ths.emplace_back(lmdk_func, 1);
	for (auto& th : ths) {
		th.join();
	}

}
