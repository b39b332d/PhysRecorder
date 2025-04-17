#include "face_tracking.h"

#include <FrameBuffer.hpp>

cv::Rect2i FaceTracking::rectify(const cv::Rect2i& dst, const cv::Size2i& size) {
	cv::Rect2i rect;
	rect.x = std::max(0, dst.x);
	rect.y = std::max(0, dst.y);
	rect.x = rect.x + dst.width;
	rect.y = rect.y + dst.height;
	rect.x = std::min(rect.x, size.width) - dst.width;
	rect.y = std::min(rect.y, size.height) - dst.height;
	rect.width = dst.width;
	rect.height = dst.height;
	return rect;
}

FaceTracking::FaceTracking(Inference* inf_backend) :inf_backend(inf_backend) {
	for (int i = 0; i < inf_backend->threads_n; i++) {
		tracking_threads.push_back(new std::thread(&FaceTracking::tracking_loop, this, i));
	}
	buffer = new Buffer;
};
FaceTracking::~FaceTracking() {
	for (auto thread : tracking_threads) {
		buffer->add(nullptr);
	}
	for (auto thread : tracking_threads) {
		thread->join();
		delete thread;
	}
	delete buffer;
	delete inf_backend;
}
cv::Rect2i FaceTracking::get_face(std::vector<FaceBox>& faces) {
	cv::Rect2i current_face;
	if (faces.size() == 0)
		return {0,0,0,0};

	// select face 
	if (last_status == TK_NOFACE) {
		std::vector<FaceBox>::iterator face = std::max_element(faces.begin(), faces.end()
			, [](const FaceBox lhs, const FaceBox& rhs) {return lhs.first.area() < rhs.first.area(); });
		current_face = face[0].first;
	}
	else {
		std::vector<FaceBox>::iterator face = std::max_element(faces.begin(), faces.end()
			, [this](const FaceBox lhs, const FaceBox& rhs) { return (lhs.first & (est_face)).area() < (rhs.first & (est_face)).area(); });
		float face_iou = (float)((face[0].first) & (est_face)).area() / ((face[0].first) | (est_face)).area();
		if (face_iou < 0.1)
			return { 0,0,0,0 };
		current_face = face[0].first;
		// kalman filter
		float face_stable_score = face_iou * face[0].second; // 0-1, 1 means the face is stable
		update_confidence(face_stable_score);

		if (face_iou > 0.95) current_face = est_face;
	}
	return current_face;

}
void FaceTracking::tracking_loop(int id) {
	while (true) {
		auto face_roi = buffer->process();
		auto frame = face_roi->frame;
		if (frame == nullptr) {
			break;
		}
		double frame_ts = (double)frame->frame_ts / 1e6;
		face_roi->ts = frame_ts;
		cv::Mat* cv_frame = (cv::Mat*)(frame->bgr_frame);
		cv::Rect2i current_face;

		std::vector<FaceBox> faces;
		RoiPoints rois;
		cv::Scalar color;
		TrackingStatus status_tmp = TK_FINISH;
		std::unique_lock<std::mutex> lock(status_lock, std::defer_lock);
		cv::Rect2i face_dst;
		float face_scale;

		lock.lock();
		if (!buffer->is_valid(face_roi))
			goto next_frame;
		if (need_reset) {
			need_reset = false;
			confidence = 0;
			last_status = TK_NOFACE;
			kf.reset();
			est_face = { 0,0,0,0 };
			on_face_lost();
		}
		if (est_face.width == 0) {
			est_face.x = 0;
			est_face.y = 0;
			est_face.width = cv_frame->cols;
			est_face.height = cv_frame->rows;
		}
		// enlarge the face
		face_dst.width = std::min(cv_frame->cols, int((1.5 * est_face.width - cv_frame->cols) * confidence + cv_frame->cols));
		face_dst.height = std::min(cv_frame->rows, int((1.5 * est_face.height - cv_frame->rows) * confidence + cv_frame->rows));
		face_dst.x = est_face.x + est_face.width / 2 - face_dst.width / 2;
		face_dst.y = est_face.y + est_face.height / 2 - face_dst.height / 2;
		lock.unlock();
		face_dst = rectify(face_dst, cv_frame->size());
		face_roi->face = face_dst;

		// inference
		face_scale = inf_backend->find_faces(face_roi, faces, id);

		lock.lock();
	next_frame:
		next_index_cond.wait(lock, 
			[&]() {return next_index == face_roi->index; });

		if (!buffer->is_valid(face_roi))
			goto release_face;
		

		// critical section
		current_face = get_face(faces);
		if (current_face.width != 0)
			kf.update(current_face, frame_ts, face_scale);
		else {
			status_tmp = TK_NOFACE;
			goto release_face;
		}

		lost_ts = -10;
		last_status = TK_FINISH;

		est_face = kf.predict();

		next_index++;
		lock.unlock();
		next_index_cond.notify_all();
		// critical section end

		face_roi->face = current_face;
		inf_backend->get_roi(face_roi, rois, id);

		lock.lock();
		next_index_output_cond.wait(lock, [&]() {return next_index_output == face_roi->index; });

		if (!buffer->is_valid(face_roi))
			goto release_roi;

		color = get_roi_color(rois, cv_frame);
		if (color[0] < 0) {
			status_tmp = TK_NOFACE;
			goto release_roi;
		}
		face_roi->r = color[2];
		face_roi->g = color[1];
		face_roi->b = color[0];

		// lock protected reset, make sure face_roi is valid
		//finish 
		////////
		face_roi->status = status_tmp;
		on_signal_ready(face_roi);

		next_index_output++;
		lock.unlock();
		next_index_output_cond.notify_all();
		// critical section end

#if DEBUG_LEVEL >0
		cv::rectangle(*cv_frame, est_face, cv::Scalar(255, 0, 0), 2);
		cv::rectangle(*cv_frame, current_face, cv::Scalar(0, 255, 0), 2);
		cv::rectangle(*cv_frame, face_dst, cv::Scalar(0, 0, 255), 2);
		cv::imshow("", *cv_frame);
		cv::waitKey(10);
#endif
		goto release_finish;


	release_face:
		next_index++;
		next_index_cond.notify_all();
		next_index_output_cond.wait(lock, [&]() {return next_index_output == face_roi->index; });

	release_roi:
		if (status_tmp == TK_NOFACE) {
			if (IS_TRACKING_SUCCESS(last_status))
				lost_ts = frame_ts;
			if (last_status != TK_NOFACE) {
				if (frame_ts - lost_ts > 0.5) {
					confidence = 0;
					last_status = TK_NOFACE;
					kf.reset();
					est_face = { 0,0,0,0 };
					on_face_lost();
				}
				else {
					update_confidence(0);
					last_status = TK_LOSS;
					est_face = kf.predict(frame_ts - lost_ts);
				}
			}
		}
		face_roi->status = status_tmp;
		on_signal_ready(face_roi);
		next_index_output++;
		lock.unlock();
		next_index_output_cond.notify_all();

	release_finish:
		frame->release();
	}
}
void FaceTracking::reset()
{
	std::unique_lock<std::mutex> lock(status_lock);
	buffer->reset();
	if (last_status != TK_STATIC || last_status != TK_DISABLE) {
		need_reset = true;
		last_status = TK_NOFACE;
		est_face = { 0,0,0,0 };
	}
}

void FaceTracking::tracking(RawFrame* frame) {
	std::unique_lock<std::mutex> lock(status_lock);
	if (last_status == TK_STATIC) {
		cv::Mat* cv_frame = (cv::Mat*)(frame->bgr_frame);
		auto o = cv::mean((*cv_frame)(est_face));
		double frame_ts = (double)frame->frame_ts / 1e6;
		auto signal = new FaceRoi{ frame_ts,o[2],o[1],o[0],{},nullptr,0,0,TK_STATIC};
		on_signal_ready(signal);
	}
	else if (last_status != TK_DISABLE) {
		buffer->add(frame);
	}
};

void FaceTracking::set_roi(cv::Rect2i roi) {
	std::unique_lock<std::mutex> lock(status_lock);
	buffer->reset();
	if (roi.width == 0 || roi.height == 0) {
		if (last_status != TK_STATIC || last_status != TK_DISABLE) {
			need_reset = true;
		}
		last_status = TK_NOFACE;
		est_face = { 0,0,0,0 };
	}
	else {
		on_face_lost();
		need_reset = true;
		est_face = roi;
		last_status = TK_STATIC;
		lock.unlock();
	}
}
void FaceTracking::disable_tracking() {
	std::unique_lock<std::mutex> lock(status_lock);
	buffer->reset();
	on_face_lost();
	if (last_status != TK_STATIC || last_status != TK_DISABLE) {
		need_reset = true;
	}
	est_face = { 0,0,0,0 };
	last_status = TK_DISABLE;
	lock.unlock();

}

cv::Scalar FaceTracking::get_roi_color(RoiPoints& rois, cv::Mat* cv_frame) {
	cv::Scalar color(0,0,0);
	// stabilize roi
	if (rois.size() == 0) {
		return {-1};
	}
	if (last_rois.size() != 0) {
		double dist = 0;
		for (int i = 0; i < last_rois.size(); i++) {
			dist += cv::norm(rois[i], last_rois[i]);

		}
		if (dist / last_rois.size() > 10) {
			last_rois = std::move(rois);
		}
	}
	else {
		last_rois = std::move(rois);
	}
	RoiPoints*  current_rois = &last_rois;

	// calculate color
	for (auto& roi : *current_rois) {
		cv::Mat1b mask(cv_frame->rows, cv_frame->cols, uchar(0));
		cv::fillPoly(mask, roi, 255);
		color += cv::mean(*cv_frame, mask);
#if DEBUG_LEVEL >0
		cv::polylines(*cv_frame, roi, true, cv::Scalar(0, 0, 255), 2);
#endif
	}
	color /= (double)current_rois->size();

	return color;
}

void FaceTracking::on_face_lost() {
	PRINT_DEBUG("face lost\n");
}

void FaceTracking::on_signal_ready(FaceRoi* signal)
{
	if (signal->status == TK_FINISH || signal->status == TK_STATIC) {
		PRINT_DEBUG("face color: %f %f %f\n", signal->r, signal->g, signal->b);
	}
	else {
		PRINT_DEBUG("face lose\n");
	}
	delete signal;
}
