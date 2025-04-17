#include "capture.h"
#include <iostream>
#include <vector>
#include <deque>
#include <qdebug>
#include "cnpy.h"
#include <QSharedPointer>
#include <algorithm>
#include <windows.h>
#include <opencv2/imgcodecs.hpp>


#include <inference_openvino.h>

#define RawFrame_CVIMG_(frame) (cv::Mat*)(frame->bgr_frame)
inline cv::Mat* raw2cvmat_bgr(RawFrame* frame) {
    if(frame->bgr_frame!= nullptr)
        return RawFrame_CVIMG_(frame);
    cv::Mat* out_image;
    if(RawFrame_FORMAT_(frame) != PIX_TYPE_BGR8)
        out_image = new cv::Mat(RawFrame_WIDTH_(frame), RawFrame_HEIGHT_(frame), CV_8UC3);
    switch (RawFrame_FORMAT_(frame)) {
    case PIX_TYPE_BGR8:  out_image = new cv::Mat(RawFrame_HEIGHT_(frame), RawFrame_WIDTH_(frame), CV_8UC3, frame->raw_frame); break;
    case PIX_TYPE_RGB8:  cv::cvtColor(cv::Mat(RawFrame_HEIGHT_(frame), RawFrame_WIDTH_(frame), CV_8UC3, frame->raw_frame), *out_image, cv::COLOR_RGB2BGR); break;
    case PIX_TYPE_RGBA:  cv::cvtColor(cv::Mat(RawFrame_HEIGHT_(frame), RawFrame_WIDTH_(frame), CV_8UC4, frame->raw_frame), *out_image, cv::COLOR_RGBA2BGR); break;
    case PIX_TYPE_BGRA:  cv::cvtColor(cv::Mat(RawFrame_HEIGHT_(frame), RawFrame_WIDTH_(frame), CV_8UC4, frame->raw_frame), *out_image, cv::COLOR_BGRA2BGR); break;

    case PIX_TYPE_RGB5:  cv::cvtColor(cv::Mat(RawFrame_HEIGHT_(frame), RawFrame_WIDTH_(frame), CV_8UC2, frame->raw_frame), *out_image, cv::COLOR_BGR5552RGB); break;
    case PIX_TYPE_BGR5:  cv::cvtColor(cv::Mat(RawFrame_HEIGHT_(frame), RawFrame_WIDTH_(frame), CV_8UC2, frame->raw_frame), *out_image, cv::COLOR_BGR5552BGR); break;
    case PIX_TYPE_RGB6:  cv::cvtColor(cv::Mat(RawFrame_HEIGHT_(frame), RawFrame_WIDTH_(frame), CV_8UC2, frame->raw_frame), *out_image, cv::COLOR_BGR5552RGB); break;
    case PIX_TYPE_BGR6:  cv::cvtColor(cv::Mat(RawFrame_HEIGHT_(frame), RawFrame_WIDTH_(frame), CV_8UC2, frame->raw_frame), *out_image, cv::COLOR_BGR5552BGR); break;

    case PIX_TYPE_YUY2:  cv::cvtColor(cv::Mat(RawFrame_HEIGHT_(frame), RawFrame_WIDTH_(frame), CV_8UC2, frame->raw_frame), *out_image, cv::COLOR_YUV2BGR_YUY2); break;
    case PIX_TYPE_NV12:  cv::cvtColor(cv::Mat(RawFrame_HEIGHT_(frame) * 1.5, RawFrame_WIDTH_(frame), CV_8UC1, frame->raw_frame), *out_image, cv::COLOR_YUV2BGR_NV12); break;

    case PIX_TYPE_UYVY:  cv::cvtColor(cv::Mat(RawFrame_HEIGHT_(frame), RawFrame_WIDTH_(frame), CV_8UC2, frame->raw_frame), *out_image, cv::COLOR_YUV2BGR_UYVY); break;
    case PIX_TYPE_Y12I:  cv::cvtColor(cv::Mat(RawFrame_HEIGHT_(frame), RawFrame_WIDTH_(frame), CV_8UC2, frame->raw_frame), *out_image, cv::COLOR_YUV2BGR_I420); break;
    case PIX_TYPE_L8:  cv::cvtColor(cv::Mat(RawFrame_HEIGHT_(frame), RawFrame_WIDTH_(frame), CV_8UC1, frame->raw_frame), *out_image, cv::COLOR_GRAY2BGR); break;
    case PIX_TYPE_L16: cv::extractChannel(cv::Mat(RawFrame_HEIGHT_(frame), RawFrame_WIDTH_(frame), CV_8UC2, frame->raw_frame), *out_image, 0);break;
    case PIX_TYPE_D16:
    case PIX_TYPE_Z16:
    {
        cv::Mat temp_img(RawFrame_HEIGHT_(frame), RawFrame_WIDTH_(frame), CV_16UC1, frame->raw_frame);
        cv::Mat log_img;
        temp_img.convertTo(log_img, CV_8UC1, 0.05);
        cv::applyColorMap(log_img, *out_image, cv::COLORMAP_RAINBOW);

    }break;
    case PIX_TYPE_MJPG:  cv::imdecode(cv::Mat(frame->raw_frame_len, 1, CV_8UC1, frame->raw_frame), cv::IMREAD_COLOR, out_image); break;
    default: return nullptr;
    };
    frame->bgr_frame = out_image;
    frame->free_funcs.push([out_image]() {delete out_image; });
    return RawFrame_CVIMG_(frame);
}




using namespace std;
using namespace cv;
using namespace cnpy;
using namespace cv::dnn;

std::mutex recorder_lock;
bool is_recording = false;
std::unordered_map<capture::CameraStream*, MediaWriter*> rec_maps;

uchar depthr_p[] = { 255,255,255,0,0,64 };
uchar depthg_p[] = { 0,165,255,255,0,64 };
uchar depthb_p[] = { 0,0,0,255,255,128 };

Capture::Capture(FaceTracking* face_tracking):face_tracking(face_tracking)
{
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
        QList<RawFrame*> mainFrames;
        cv::Mat color_mat;
        RawFrame* c_frame;
        for (auto devices : enabled_devices) {
            for (auto stream : devices->enabled_streams) {
                if (frame_set.contains(stream)) {
                    auto frame = frame_set[stream].back();
                    cv::Mat* temp_mat = raw2cvmat_bgr(frame);
                    frame->acquire();
                    if (selected_stream == stream) {
                        if (color_mat.empty()) {
                            color_mat = *temp_mat;
                            c_frame = frame;
                        }
                        mainFrames.push_back(frame);
                    }
                    else
                        otherFrames.push_back(frame);
                }
                else {
                    RawFrame* frame_place_holder = new RawFrame;
                    frame_place_holder->profile = stream->selected_profile;
                    if (selected_stream == stream) {
                        mainFrames.push_back(frame_place_holder);
                    }
                    else
                        otherFrames.push_back(frame_place_holder);
                }
            }
        }

        int rot_current = rot;
        cv::Mat rot_mat;
        if (color_mat.empty() ) {
            goto imp_finish;
        }
        // rotation
        if (rot_current != 0) {
            // get rotation matrix for rotating the image around its center in pixel coordinates
            cv::Point2f center((color_mat.cols - 1) / 2.0, (color_mat.rows - 1) / 2.0);
            rot_mat = cv::getRotationMatrix2D(center, rot_current, 1.0);
            cv::Mat* dst = new cv::Mat;
            cv::warpAffine(color_mat, *dst, rot_mat, color_mat.size());
            color_mat = *dst;
            c_frame->bgr_frame = dst;
            c_frame->free_funcs.push([dst]() {delete dst; });
        }
        // flip
        if (is_fliplr)
            if(is_flipud)cv::flip(color_mat, color_mat, -1);
            else cv::flip(color_mat, color_mat, 1);
        else
            if(is_flipud)cv::flip(color_mat, color_mat, 0);

		// tracking

        static capture::CameraStream* current_stream = nullptr;
        if (selected_stream != current_stream) {
            current_stream = selected_stream;
            face_tracking->reset();
        }
		face_tracking->tracking(c_frame);
        imp_finish:
        emit updateFrame(mainFrames, otherFrames, face_tracking->get_roi());

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
