#include "capture.h"
#include <iostream>
#include <vector>
#include <deque>
#include <qdebug>
#include "cnpy.h"
#include <QSharedPointer>
#include <algorithm>
#include <windows.h>

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

    case PIX_TYPE_YUYV:  cv::cvtColor(cv::Mat(RawFrame_HEIGHT_(frame), RawFrame_WIDTH_(frame), CV_8UC2, frame->raw_frame), *out_image, cv::COLOR_YUV2BGR_YUYV); break;
    case PIX_TYPE_UYVY:  cv::cvtColor(cv::Mat(RawFrame_HEIGHT_(frame), RawFrame_WIDTH_(frame), CV_8UC2, frame->raw_frame), *out_image, cv::COLOR_YUV2BGR_UYVY); break;
    case PIX_TYPE_Y12I:  cv::cvtColor(cv::Mat(RawFrame_HEIGHT_(frame), RawFrame_WIDTH_(frame), CV_8UC2, frame->raw_frame), *out_image, cv::COLOR_YUV2BGR_I420); break;
    case PIX_TYPE_L8:  cv::cvtColor(cv::Mat(RawFrame_HEIGHT_(frame), RawFrame_WIDTH_(frame), CV_8UC1, frame->raw_frame), *out_image, cv::COLOR_GRAY2BGR); break;
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
std::string record_prefix;
bool is_recording = false;
std::unordered_map<capture::CameraStream*, std::pair<MediaWriter*, std::vector<double>*>> rec_maps;

uchar depthr_p[] = { 255,255,255,0,0,64 };
uchar depthg_p[] = { 0,165,255,255,0,64 };
uchar depthb_p[] = { 0,0,0,255,255,128 };

Capture::Capture(Converter& converter, SignalProcess* sp) :
    converter(converter),
    tracking(false),
    detect_mode(0), // 0->manual 1->center 2-> auto
    interp_cyc(10),
    timestamp_line_count(0),
    capture_ready(0),
    timestamp_method(0),
    signalProcess(sp)
{

    initInferenceEngine();
    QObject::connect(this, &Capture::updateFrame, &converter, &Converter::frame_ready);

    connect(this, &Capture::loseTracking, sp, &SignalProcess::reset);

    //In order to begin getting data from the sensor, we need to register a class to handle frames, 
    // in our case we provide the frame_queue when starting the sensor.
    depthLUT = Mat(Size(1, 256*256), CV_8UC4);
    int j = 0,i=0;
    for (; j < 5; j++) {
        for (; i < 13107; i++) {
            depthLUT.data[(j * 13107 + i) * 4 + 0] = (depthb_p[j + 1] - depthb_p[j]) * ((double)i / 13107) + depthb_p[j];
            depthLUT.data[(j * 13107 + i) * 4 + 1] = (depthg_p[j + 1] - depthg_p[j]) * ((double)i / 13107) + depthg_p[j];
            depthLUT.data[(j * 13107 + i) * 4 + 2] = (depthr_p[j + 1] - depthr_p[j]) * ((double)i / 13107) + depthr_p[j];
        }
    }
    depthLUT.data[(j * 13107 + i) * 4 + 0] = depthb_p[5];
    depthLUT.data[(j * 13107 + i) * 4 + 1] = depthg_p[5];
    depthLUT.data[(j * 13107 + i) * 4 + 2] = depthr_p[5];
}




static cv::Rect2i getSquareBox(cv::Rect2i& face, cv::Size frame_size, double scale = 1.2) {
    float max_size = max(face.width, face.height) * scale;
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
    return Rect2i(center_x - max_size / 2,
        center_y - max_size / 2, max_size, max_size);
}

cv::Mat Capture::LUT_16_reinterpret_cast(cv::Mat mat, cv::Mat dst)
{
    int limit = mat.rows * mat.cols;
    ushort* ptr = reinterpret_cast<ushort*>(mat.data);
    uchar* ptr_d = reinterpret_cast<uchar*>(dst.data);
    uint32_t* ptr_t = (uint32_t*)(depthLUT.data);
    for (int i = 0; i < limit-1; i++, ptr++, ptr_d+=3)
    {
        *(uint32_t*)ptr_d = ptr_t[*ptr];
    }
    *(ushort*)ptr_d = (ushort)ptr_t[*ptr];
    *(ptr_d+2) = *( (uchar*)(ptr_t+*ptr) + 2);
    return mat;
}
void Capture::run()
{
    int i_loop = 0;
    double last_ts = 0;
    double last_ts2 = 0;
    double fps = 0;
    Rect2i* rect_face=NULL;

    int error_cnt = 0;
    static bool last_tracking_status = true;
    emit loseTracking();

    int sync_stage = 0;
    bool is_sync_generated = false;
    cv::Mat sync_pic;
    int sync_width;
    vector<double> sync_ofs;
    double sync_ts_save = 0;
    bool last_sync_status = false;

    int f_cnt = 0;
    bool first_image = false;
    cv::AsyncArray face_landmarks_last;
    face_landmarks_last.release();
    cv::Mat crop_face_last;
    double crop_face_last_ts;
    auto next_refresh_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(40);
    while (true)
    {
        capture::frame_set_t frame_set;
        std::set<capture::CameraDevice*> new_disabled_devices;
        std::set<capture::CameraDevice*> enabled_devices;
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
                    if (selected_device == devices) {
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
                    if (selected_device == devices) {
                        mainFrames.push_back(frame_place_holder);
                    }
                    else
                        otherFrames.push_back(frame_place_holder);
                }
            }
        }


        int rot_current = rot;
        cv::Mat rot_mat;
        if (color_mat.empty()) {
            goto track_finish;
        }
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
        if (is_fliplr)
            if(is_flipud)cv::flip(color_mat, color_mat, -1);
            else cv::flip(color_mat, color_mat, 1);
        else
            if(is_flipud)cv::flip(color_mat, color_mat, 0);

        if (tracking) {
            error_cnt++;
            if (!last_tracking_status) {
                emit loseTracking(); 
                last_tracking_status = true;
            }
            net_face.setInput(dnn::blobFromImage(color_mat, 1, Size(300, 300)));
            Mat prob = net_face.forward();
            Mat preds_face(prob.size[2], prob.size[3], CV_32F, prob.ptr<float>());
            vector<Rect2i> roi_faces;
            for (uchar row = 0; row < preds_face.size[0]; row++) {
                const float* pred = preds_face.ptr<float>(row);
                if (pred[2] > 0.8) {
                    roi_faces.emplace_back(Rect2i(Point2i(pred[3] * color_mat.cols, pred[4] * color_mat.rows), Point2i(pred[5] * color_mat.cols, pred[6] * color_mat.rows)));
                }
                else break;
            }
            if (error_cnt >= 30) {
                delete rect_face;
                rect_face = NULL;
                error_cnt = 0;
                emit loseTracking();
            }
            if (roi_faces.size() == 0 && rect_face == NULL) {
                goto track_finish;
            }
            else if (roi_faces.size() == 0 && rect_face != NULL) {
                ;
            }
            else if (roi_faces.size() > 0) {
                if (rect_face == NULL) {
                    std::vector<Rect2i>::iterator face = std::max_element(roi_faces.begin(), roi_faces.end()
                        , [](Rect2i lhs, Rect2i rhs) {return lhs.area() < rhs.area(); });
                    if (face[0].area() == 0){
                        goto track_finish;
                    }
                    else {
                        rect_face = new Rect2i(face[0]);
                        error_cnt = 0;
                    }
                }
                else {
                    std::vector<Rect2i>::iterator face = std::max_element(roi_faces.begin(), roi_faces.end()
                        , [rect_face](Rect2i lhs, Rect2i rhs) { return (lhs & (*rect_face)).area() < (rhs & (*rect_face)).area(); });
                    if (((face[0]) & (*rect_face)).area() > 0) {
                        error_cnt = 0;
                        delete rect_face;
                        rect_face = new Rect2i(face[0]);
                    }
                }
            }
            auto crop_face = color_mat(getSquareBox(*rect_face, color_mat.size(), 1.2));
            net_landmarks.setInput(dnn::blobFromImage(crop_face, 1, Size(60, 60)));
            Mat raw_preds_landmarks, face_region;
            double crop_face_ts;
            if (face_landmarks_last.valid()) {
                face_landmarks_last.get(raw_preds_landmarks);
                face_region = std::move(crop_face_last);
                crop_face.copyTo(crop_face_last);
                crop_face_ts = crop_face_last_ts;
                crop_face_last_ts = (double)(mainFrames.front()->frame_ts)/1e6 ;
            }
            else {
                face_landmarks_last = net_landmarks.forwardAsync();
                crop_face_last_ts = (double)(mainFrames.front()->frame_ts) / 1e6;
                crop_face.copyTo(crop_face_last);
                goto track_finish;
            }
            raw_preds_landmarks *= face_region.rows;
            const float* pred = raw_preds_landmarks.ptr<float>(0);

            Point face_left_top(pred[38], pred[39]);
            Point face_right_top(pred[66], pred[67]);
            if (pred[0] < 0.00001 && pred[1] < 0.0001||face_left_top.x - face_right_top.x >= 0) {
                error_cnt = 30;
                goto track_finish;
            }
            float k = (face_left_top.y - face_right_top.y) / (face_left_top.x - face_right_top.x);
            float b = face_left_top.y - k * face_left_top.x;
            Point face_6(pred[12], pred[13]), face_7(pred[14], pred[15]);
            float x = (face_6.x + (face_6.y - b) * k) / (1 + k * k);
            Point face_center_left(x, k * x + b);
            x = (face_7.x + (face_7.y - b) * k) / (1 + k * k);
            Point face_center_right(x, k * x + b);

            vector<cv::Point> pface1{ face_6, face_center_left ,Point((pred[40] + pred[38]) / 2, (pred[41] + pred[39]) / 2) ,
                                Point(pred[40],pred[41]), Point(pred[42],pred[43]),
                                Point(pred[44],pred[45]), Point(pred[46],pred[47]),
                                Point(pred[16],pred[17]), };
            vector<cv::Point> pface2 = { Point((pred[66] + pred[64]) / 2, (pred[67] + pred[65]) / 2), face_center_right ,face_7,
                                Point(pred[18],pred[19]), Point(pred[58],pred[59]),
                                Point(pred[60],pred[61]), Point(pred[62],pred[63]),
                                Point(pred[64],pred[65]), };
            //8uc3 or 8u ,size()/3?
            Mat mask = Mat::zeros(face_region.size(), CV_8U);
            cv::fillPoly(mask, pface1, 255);
            cv::fillPoly(mask, pface2, 255);
            emit signalReady(Scalar(mean(face_region, mask)), crop_face_ts);

        }
        else {
            if (face_landmarks_last.valid()) {
                face_landmarks_last.release();
            }
            last_tracking_status = false;
            if (rect_face != nullptr) {
                delete rect_face;
                rect_face = nullptr;
            }
        }

    track_finish:
        if(rect_face == nullptr)
            emit updateFrame(mainFrames, otherFrames, {});
        else
            emit updateFrame(mainFrames, otherFrames,  *rect_face);


        recorder_lock.lock();
        if (is_recording) {
            for (auto& [stream, rec] : rec_maps) {
                for (auto frame : frame_set[stream]) {
                    frame->acquire();
                    rec.first->write(frame);
                    rec.second->push_back((float)(frame->frame_ts)/1e6 - signalProcess->cam_ofs);
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


void Capture::initInferenceEngine() {
    // command of downloading from open_model_zoo: 
    // omz_downloader  --name face-detection-retail-0005 --output_dir C:\Users\b39b3\Documents\src\opencv\modules --precisions FP16,FP16-INT8,FP32
    string root_path = PROJECT_ROOT_PATH;
    string path_net_facedetect = root_path + "models/intel/face-detection-retail-0005/FP32/face-detection-retail-0005";
    string path_net_landmarks = root_path + "models/intel/facial-landmarks-35-adas-0002/FP32/facial-landmarks-35-adas-0002";
    string resource_path = root_path + "resources/";

    net_face = readNetFromModelOptimizer(path_net_facedetect + ".xml", path_net_facedetect + ".bin");
    net_face.setPreferableBackend(DNN_BACKEND_INFERENCE_ENGINE);
    net_face.setPreferableTarget(DNN_TARGET_CPU);
    net_face.setInput(dnn::blobFromImage(imread(resource_path + "faces.jpg"), 1, Size(300, 300)));
    net_face.forward();

    net_landmarks = readNetFromModelOptimizer(path_net_landmarks + ".xml", path_net_landmarks + ".bin");
    net_landmarks.setPreferableBackend(DNN_BACKEND_INFERENCE_ENGINE);
    net_landmarks.setPreferableTarget(DNN_TARGET_CPU);
    Mat test_face = imread(resource_path + "face.jpg");
    net_landmarks.setInput(dnn::blobFromImage(test_face, 1, Size(60, 60)));
    auto w = net_landmarks.forwardAsync();
    cv:Mat out;
    w.get(out);
}