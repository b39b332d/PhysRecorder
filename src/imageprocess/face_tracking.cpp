#include <face_tracking.h>

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
}
void FaceTracking::tracking_loop(int id) {
	while (true) {
		auto face_roi = buffer->process();
		auto frame = face_roi->frame;
		if (frame == nullptr) {
			while (next_index.load() != face_roi->index)
				next_index.wait(next_index.load());
			next_index++;
			next_index.notify_all();
			break;
		}
		double frame_ts = (double)frame->frame_ts / 1e6;
		cv::Mat* cv_frame = (cv::Mat*)(frame->bgr_frame);
		cv::Rect2i current_face;

		std::vector<FaceBox> faces;
		std::vector < std::vector<cv::Point2i>> rois;
		std::vector < std::vector<cv::Point2i>>* current_rois;

		status_lock.lock();
		if (est_face.width == 0) {
			est_face.x = 0;
			est_face.y = 0;
			est_face.width = cv_frame->cols;
			est_face.height = cv_frame->rows;
		}

		// enlarge the face
		cv::Rect2i face_dst;
		face_dst.width = std::min(cv_frame->cols, int((1.5 * est_face.width - cv_frame->cols) * confidence + cv_frame->cols));
		face_dst.height = std::min(cv_frame->rows, int((1.5 * est_face.height - cv_frame->rows) * confidence + cv_frame->rows));
		face_dst.x = est_face.x + est_face.width / 2 - face_dst.width / 2;
		face_dst.y = est_face.y + est_face.height / 2 - face_dst.height / 2;
		status_lock.unlock();
		auto est_face_reg = rectify(face_dst, cv_frame->size());
		face_roi->face = est_face_reg;

		// inference
		float face_scale = inf_backend->find_faces(face_roi, faces, id);

		while (next_index.load() != face_roi->index)
			next_index.wait(next_index.load());

		// critical section
		TrackingStatus status_tmp = TRACKING_FIN;
		if (faces.size() == 0)
			goto noface_release;

		// select face 
		if (last_status == TRACKING_NOF) {
			std::vector<FaceBox>::iterator face = std::max_element(faces.begin(), faces.end()
				, [](const FaceBox lhs, const FaceBox& rhs) {return lhs.first.area() < rhs.first.area(); });
			current_face = face[0].first;
			kf.init(current_face, frame_ts, face_scale);
		}
		else {
			std::vector<FaceBox>::iterator face = std::max_element(faces.begin(), faces.end()
				, [this](const FaceBox lhs, const FaceBox& rhs) { return (lhs.first & (est_face)).area() < (rhs.first & (est_face)).area(); });
			float face_iou = (float)((face[0].first) & (est_face)).area() / ((face[0].first) | (est_face)).area();
			if (face_iou < 0.1)
				goto noface_release;
			current_face = face[0].first;
			// kalman filter
			kf.update(current_face, frame_ts, face_scale);
			float face_stable_score = face_iou * face[0].second; // 0-1, 1 means the face is stable
			update_confidence(face_stable_score);

			if (face_iou > 0.95) current_face = est_face;
		}
		last_status = TRACKING_FIN;
		est_face = kf.predict();

		next_index++;
		next_index.notify_all();
		// critical section end

		face_roi->face = current_face;
		inf_backend->get_roi(face_roi, rois, id);
		if (rois.size() == 0) {
			face_roi->status = TRACKING_LOS;
			goto release_frame;
		}


		// stabilize roi
		rois_lock.lock();
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
	rois_selected:
		current_rois = &last_rois;
		rois_lock.unlock();

		// calculate color
		for (auto& roi : *current_rois) {
			cv::Mat1b mask(cv_frame->rows, cv_frame->cols, uchar(0));
			cv::fillPoly(mask, roi, 255);
			auto val = cv::mean(*cv_frame, mask);
			face_roi->r += val[2];
			face_roi->g += val[1];
			face_roi->b += val[0];
			cv::polylines(*cv_frame, roi, true, cv::Scalar(0, 0, 255), 2);
		}
		face_roi->r /= current_rois->size();
		face_roi->g /= current_rois->size();
		face_roi->b /= current_rois->size();

		//cv::rectangle(*cv_frame, est_face, cv::Scalar(255, 0, 0), 2);
		//cv::rectangle(*cv_frame, current_face, cv::Scalar(0, 255, 0), 2);
		//cv::rectangle(*cv_frame, est_face_reg, cv::Scalar(0, 0, 255), 2);
		//cv::imshow("", *cv_frame);
		//cv::waitKey(10);

		lost_ts = -10;
		face_roi->status = TRACKING_FIN;
		face_roi->face = current_face;
		goto release_frame;

	noface_release:
		if (last_status >= 0)
			lost_ts = frame_ts;
		face_roi->status = TRACKING_LOS;
		est_face = kf.predict(frame_ts - lost_ts);
		next_index++;
		next_index.notify_all();


	release_frame:
		status_lock.lock();
		if (face_roi->status < 0) {
			if (frame_ts - lost_ts > 0.5) {
				est_face.width = 0;
				confidence = 0;
				face_roi->status = TRACKING_NOF;
				last_status = TRACKING_NOF;
				last_rois.clear();
			}
			else {
				update_confidence(0);
				face_roi->status = TRACKING_LOS;
				last_status = TRACKING_LOS;
			}
		}
		status_lock.unlock();
		//release frame
		face_roi->ts = frame_ts;
		frame->release();
		face_roi->status = status_tmp;
		buffer->put(face_roi);
	}
};


std::vector<FaceRoi*> FaceTracking::tracking(RawFrame* frame) {
	buffer->add(frame);
	return buffer->get();
};