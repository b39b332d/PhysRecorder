#include "capture.h"
#include <iostream>
#include <vector>
#include <deque>
#include <qdebug>
#include "cnpy.h"
#include <QSharedPointer>
#include <algorithm>
#include <windows.h>
#include <inference_openvino.h>
#include <ImageDecoder.h>



using namespace std;
using namespace cv;
using namespace cnpy;
using namespace cv::dnn;

Capture::Capture(FaceTracking* face_tracking):face_tracking(face_tracking)
{
	th = new std::thread(&Capture::run, this);
}

void Capture::setInference(FaceTracking* ft)
{
    if (face_tracking != nullptr) {
        tracking_lock.lock();
        face_tracking = nullptr;
        tracking_lock.unlock();
    }
    face_tracking = ft;
}

void Capture::startRecord(bool is_start)
{
    recorder_lock.lock();
    is_recording = is_start;
    recorder_lock.unlock();
    if (!is_start) {
        for (auto& [stream, rec] : rec_maps) delete rec;
        rec_maps.clear();
    }
}

void Capture::recordStream(capture::CameraStream* stream, const std::string& file_name)
{
    auto v_rec = new MediaWriter(file_name,
        stream->resolution, stream->ratio, stream->format,
        stream->encoder_method, stream->encoder_quality);
    rec_maps[stream] = v_rec;
}

//cv::Mat Capture::LUT_16_reinterpret_cast(cv::Mat mat, cv::Mat dst)
//{
//    int limit = mat.rows * mat.cols;
//    ushort* ptr = reinterpret_cast<ushort*>(mat.data);
//    uchar* ptr_d = reinterpret_cast<uchar*>(dst.data);
//    uint32_t* ptr_t = (uint32_t*)(depthLUT.data);
//    for (int i = 0; i < limit-1; i++, ptr++, ptr_d+=3)
//    {
//        *(uint32_t*)ptr_d = ptr_t[*ptr];
//    }
//    *(ushort*)ptr_d = (ushort)ptr_t[*ptr];
//    *(ptr_d+2) = *( (uchar*)(ptr_t+*ptr) + 2);
//    return mat;
//}
void Capture::run()
{
    auto next_refresh_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(40);
    while (true)
    {
        capture::frame_set_t frame_set;
        capture::devices_set_t new_disabled_devices;
        capture::devices_set_t enabled_devices;
        capture::readFrames(frame_set,
            next_refresh_time,
            new_disabled_devices, enabled_devices);
         next_refresh_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(50);

         for (auto d : new_disabled_devices) {
             emit device_disabled(d);
         }
        RawFrame* raw_frame = NULL;
        QList<RawFrame*> otherFrames;
        cv::Mat color_mat;
        RawFrame* c_frame=nullptr;
        for (auto devices : enabled_devices) {
            for (auto stream : devices->enabled_streams) {
                if (frame_set.contains(stream)) {
                    auto frame = frame_set[stream].back();
                    cv::Mat temp_mat;
                    if (frame->bgr_frame == nullptr) {
                        temp_mat = capture::decode_bgr(frame);
                        frame->bgr_frame = new cv::Mat(temp_mat);
                        frame->free_funcs.push([frame]() { delete (cv::Mat*)frame->bgr_frame; });
                    }else
						temp_mat = *(cv::Mat*)(frame->bgr_frame);
                    frame->acquire();
                    if (selected_stream == stream) {
                        color_mat = temp_mat;
                        c_frame = frame;
                    }
                    else
                        otherFrames.push_back(frame);
                }
                else {
                    RawFrame* frame_place_holder = stream->createEmptyFrame();
                    if (selected_stream == stream) {
                        c_frame = frame_place_holder;
                    }
                    else
                        otherFrames.push_back(frame_place_holder);
                }
            }
        }
        RawFrame* show_frame = c_frame;
        int rot_current = rot;
        cv::Mat show_mat;
        cv::Rect2i face_region;
        cv::Mat rot_mat;

        if (c_frame == nullptr || c_frame->is_empty()) {
            face_region = { 0,0,0,0 };
            goto imp_finish;
        }

        if (rot_current != 0 || scale != 1.0) {
            // get rotation matrix for rotating the image around its center in pixel coordinates
            cv::Point2f center((color_mat.cols - 1) / 2.0, (color_mat.rows - 1) / 2.0);
            rot_mat = cv::getRotationMatrix2D(center, rot_current, scale);
            // determine bounding rectangle, center not relevant
            cv::Rect2f bbox = cv::RotatedRect(cv::Point2f(), color_mat.size(), rot_current).boundingRect2f();
            cv::Size2i vid_size_show = bbox.size();
            // adjust transformation matrix
            rot_mat.at<double>(0, 2) += bbox.width / 2.0 - color_mat.cols / 2.0;
            rot_mat.at<double>(1, 2) += bbox.height / 2.0 - color_mat.rows / 2.0;

            cv::warpAffine(color_mat, show_mat, rot_mat, vid_size_show);
        }


        // flip
        if (is_fliplr)
            if(is_flipud) cv::flip(rot_current != 0 ? show_mat: color_mat, show_mat, -1);
            else cv::flip(rot_current != 0 ? show_mat : color_mat, show_mat, 1);
        else
            if(is_flipud)cv::flip(rot_current != 0 ? show_mat : color_mat, show_mat, 0);
        if (!show_mat.empty()) {
            auto t = new cv::Mat(show_mat);
            show_frame = new RawFrame{ t->data,(unsigned)(t->datastart-t->dataend),Resolution{(unsigned)t->cols,(unsigned)t->rows},PIX_TYPE_BGR8,c_frame->frame_ts,c_frame->profile,1,t };
            show_frame->free_funcs.push([c_frame, t]() {c_frame->release(); delete t; });
        }


		// tracking
        tracking_lock.lock();
        if(face_tracking!= nullptr){
            if (selected_stream != last_stream) {
                last_stream = selected_stream;
                face_tracking->reset();
            }

            qDebug() << show_frame->frame_ts;
		    face_tracking->tracking(show_frame);
            face_region = face_tracking->get_roi();
        }
        tracking_lock.unlock();

        imp_finish:
        emit updateFrame(show_frame, otherFrames, face_region);

        recorder_lock.lock();
        if (is_recording) {
            for (auto& [stream, rec] : rec_maps) {
                for (auto frame : frame_set[stream]) {
                    frame->acquire();
                    rec->write(frame);
                }
            }
        }
        recorder_lock.unlock();


        for (auto [_, frames] : frame_set) {
            for (auto f : frames) {
                f->release();
            }
        }
    }
}
