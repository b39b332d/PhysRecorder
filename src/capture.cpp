#include "capture.h"
#include <iostream>
#include <vector>
#include <deque>
#include <qdebug>
#include "cnpy.h"
#include <QSharedPointer>
#include <algorithm>
#include <librealsense2/h/rs_types.h>
#include <windows.h>
using namespace std;
using namespace cv;
using namespace cnpy;
using namespace cv::dnn;


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
    signalProcess(sp),
    fq(1),
    sensor(NULL)
{

    initInferenceEngine();
    QObject::connect(this, &Capture::facesReady, &converter, &Converter::processFaces);
    QObject::connect(this, &Capture::faceReady, &converter, &Converter::processFace);
    QObject::connect(this, &Capture::updateFrame, &converter, &Converter::updateFrame);

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


void Capture::setCapture(rs2::sensor& sensor,int width,int height,int fps) {
    use_camera = false;
    this->sensor = &sensor;
    this->vid_size = Size2i(width, height);
    this->rec_fps = fps;
}

void Capture::setCVCamProperty(int propId, double value)
{
    cam_cap.set(propId, value);
}

void Capture::setCamera(int cam_idx, int width, int height, float fps)
{
    use_camera = true;
    this->cam_idx = cam_idx;
    vid_size = Size2i(width, height);
    this->rec_fps = fps;
}


void Capture::stop() {
    suicide = true;
}

cv::Rect2i getSquareBox(cv::Rect2i& face, cv::Size frame_size, double scale = 1.2) {
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

void Capture::wait_for_rec_save() {
    if (this->isRunning == false)
        return;
    recorder_lock.lock();
    (*rec).release();
    delete rec;
    rec = NULL;
    string fname = "./rec/";
    cnpy::npy_save(fname+ string(save_path) + "vid_ts.npy", rec_ts);
    rec_ts.clear();
    recorder_lock.unlock();
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
    int rot_prev = 0;
    cv::Size2i vid_size_rot = vid_size;
    cv::Mat rot_mat;
    Rect2i* rect_face=NULL;
    double ts_offset_boot;
    if (use_camera) {
        cam_cap.open(cam_idx, cv::CAP_MSMF);
        cam_cap.set(cv::CAP_PROP_FRAME_WIDTH, vid_size.width);
        cam_cap.set(cv::CAP_PROP_FRAME_HEIGHT, vid_size.height);
        cam_cap.set(cv::CAP_PROP_FPS, rec_fps);
        ts_offset_boot = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count() - 1.0 * GetTickCount64() / 1000;
    }
    else {
        sensor->start(fq);
    }
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
    cv::Mat color_mat(vid_size,CV_8UC3);
    bool first_image = false;

    while (true)
    {

        if (isRunning == false) {
            break;
        }
        double color_mat_ts;
        if (use_camera) {
            bool read_success = cam_cap.read(color_mat);
            if (!read_success)
                break;
            color_mat_ts = cam_cap.get(cv::CAP_PROP_POS_MSEC) / 1000.0 + ts_offset_boot;
        }
        else {
            rs2::frame f = fq.wait_for_frame();
            auto fmt = f.get_profile().format();
            if (fmt == rs2_format::RS2_FORMAT_BGR8)
                color_mat = Mat(this->vid_size, CV_8UC3, (void*)f.get_data(), Mat::AUTO_STEP);
            else if (fmt == rs2_format::RS2_FORMAT_Y8)
                cvtColor(Mat(this->vid_size, CV_8UC1, (void*)f.get_data(), Mat::AUTO_STEP), color_mat, cv::COLOR_GRAY2BGR);
            else if (fmt == rs2_format::RS2_FORMAT_Z16) {
                //convertScaleAbs(Mat(this->vid_size, CV_16UC1, (void*)f.get_data(), Mat::AUTO_STEP), color_mat, 0.03);
                //applyColorMap(color_mat, color_mat, COLORMAP_JET);
                Mat different_Channels[2];
                split(Mat(this->vid_size, CV_8UC2, (void*)f.get_data(), Mat::AUTO_STEP), different_Channels);
                merge(vector<Mat>{different_Channels[0], different_Channels[1], (different_Channels[0] + different_Channels[1]) / 2}, color_mat);
                //LUT_16_reinterpret_cast(Mat(this->vid_size, CV_16UC1, (void*)f.get_data(), Mat::AUTO_STEP), color_mat);
                //imshow("", color_mat);
                //waitKey(1);
            }
            color_mat_ts = f.get_timestamp() / 1000.0;
        }
        if (!first_image) {
            first_image = true;
            emit cap_started();
        }
        // syncing
        if (is_syncing) {
            last_sync_status = true;
            if (sync_stage%2 == 0) {
                if (!is_sync_generated) {
                    sync_pic = Mat::zeros(color_mat.size(), CV_8UC3);
                    sync_width = min(color_mat.rows, color_mat.cols);
                    cv::resize(sync_stage1, Mat(sync_pic, cv::Rect(0, 0, sync_width, sync_width)), Size(sync_width, sync_width));
                    emit updateFrame(sync_pic);
                    is_sync_generated = true;
                }
                cv::cvtColor(color_mat, color_mat, COLOR_BGR2GRAY);
                cv::resize(color_mat, color_mat, Size(), 0.25, 0.25);
                std::string data = qrDecoder.detectAndDecode(color_mat);
                
                if (data == "stage1") {
                    if(sync_ts_save!=0 ){
                        sync_ofs.push_back((color_mat_ts - sync_ts_save) - 1.0/ rec_fps);
                        sync_ts_save = color_mat_ts;
                    }
                    sync_stage++;
                    cv::resize(sync_stage2, Mat(sync_pic, cv::Rect(0, 0, sync_width, sync_width)), Size(sync_width, sync_width));
                    emit updateFrame(sync_pic);
                    sync_ts_save = color_mat_ts;
                }
            }
            else{
                cv::cvtColor(color_mat, color_mat, COLOR_BGR2GRAY);
                cv::resize(color_mat, color_mat,Size(),0.25,0.25);
                std::string data = qrDecoder.detectAndDecode(color_mat);
                if (data == "2stage_sync") {
                    sync_stage++;
                    sync_ofs.push_back((color_mat_ts - sync_ts_save) - 1.0 / rec_fps);
                    sync_ts_save = color_mat_ts;
                    if (sync_stage == 10) {
                        is_sync_generated = false;
                        emit tsofsReady(*min_element(sync_ofs.begin(), sync_ofs.end()));
                    }
                    else {
                        cv::resize(sync_stage1, Mat(sync_pic, cv::Rect(0, 0, sync_width, sync_width)), Size(sync_width, sync_width));
                        emit updateFrame(sync_pic);

                    }
                }
            }
            continue;
        }
        else if (last_sync_status) {
            last_sync_status = false;
            sync_ofs.clear();
            sync_stage = 0;
            is_sync_generated = false;
        }




        int rot_current = rot;
        if (rot_current != rot_prev) {
            if (rot_current != 0) {
                // get rotation matrix for rotating the image around its center in pixel coordinates
                cv::Point2f center((color_mat.cols - 1) / 2.0, (color_mat.rows - 1) / 2.0);
                rot_mat = cv::getRotationMatrix2D(center, rot_current, 1.0);
                // determine bounding rectangle, center not relevant
                cv::Rect2f bbox = cv::RotatedRect(cv::Point2f(), color_mat.size(), rot_current).boundingRect2f();
                vid_size_rot = bbox.size();
                // adjust transformation matrix
                rot_mat.at<double>(0, 2) += bbox.width / 2.0 - color_mat.cols / 2.0;
                rot_mat.at<double>(1, 2) += bbox.height / 2.0 - color_mat.rows / 2.0;

            }
            else {
                vid_size_rot = vid_size;
            }
            rot_prev = rot_current;
        }
        if (rot_prev != 0) {
            cv::Mat dst;
            cv::warpAffine(color_mat, dst, rot_mat, vid_size_rot);
            color_mat = dst;
        }
        if (is_fliplr)
            if(is_flipud)cv::flip(color_mat, color_mat, -1);
            else cv::flip(color_mat, color_mat, 1);
        else
            if(is_flipud)cv::flip(color_mat, color_mat, 0);
        //std::cout << std::chrono::duration<double, std::milli>(std::chrono::system_clock::now().time_since_epoch()).count() - f.get_timestamp();
        //std::cout << f.get_frame_timestamp_domain() << std::endl;
        // Do something with the received frame
        i_loop++;
        if (i_loop % 10 == 0) {
            fps = 10.0 / (color_mat_ts - last_ts);
            last_ts = color_mat_ts;
            emit fpsReady(fps, color_mat.rows, color_mat.cols);
        }
        recorder_lock.lock();
        if (isRecording) {
            if (rec == NULL) {
                string fname = "./rec/";
                rec = new VideoWriter(fname + string(save_path) + "vid.avi", cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), rec_fps, color_mat.size());
            }
            (*rec).write(color_mat);
            rec_ts.push_back(color_mat_ts- signalProcess->cam_ofs);
        }
        recorder_lock.unlock();

        if (tracking) {
            if (!last_tracking_status) {
                emit loseTracking(); last_tracking_status = true;
            }
            net_face.setInput(dnn::blobFromImage(color_mat, 1, Size(300, 300)));
            Mat prob = net_face.forward();
            Mat preds_face(prob.size[2], prob.size[3], CV_32F, prob.ptr<float>());
            vector<Rect2i> roi_faces;
            for (uchar row = 0; row < preds_face.size[0]; row++) {
                const float* pred = preds_face.ptr<float>(row);
                if (pred[2] > 0.8) {
                    roi_faces.emplace_back(Rect2i(Point2i(pred[3] * vid_size_rot.width, pred[4] * vid_size_rot.height), Point2i(pred[5] * vid_size_rot.width, pred[6] * vid_size_rot.height)));
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
                emit updateFrame(color_mat);
                continue;
            }
            else if (roi_faces.size() == 0 && rect_face != NULL) {
                error_cnt++;
            }
            else if (roi_faces.size() > 0) {
                if (rect_face == NULL) {
                    std::vector<Rect2i>::iterator face = std::max_element(roi_faces.begin(), roi_faces.end()
                        , [](Rect2i lhs, Rect2i rhs) {return lhs.area() < rhs.area(); });
                    rect_face = new Rect2i(face[0]);
                    error_cnt = 0;
                }
                else {
                    std::vector<Rect2i>::iterator face = std::max_element(roi_faces.begin(), roi_faces.end()
                        , [rect_face](Rect2i lhs, Rect2i rhs) { return (lhs & (*rect_face)).area() < (rhs & (*rect_face)).area(); });
                    if (((face[0]) & (*rect_face)).area() > 0) {
                        error_cnt = 0;
                        delete rect_face;
                        rect_face = new Rect2i(face[0]);
                    }
                    else {
                        error_cnt++;
                    }
                }
            }
            emit faceReady(color_mat, Rect(rect_face->tl(), rect_face->br()));
            auto crop_face = color_mat(getSquareBox(*rect_face, vid_size_rot, 1.2));
            net_landmarks.setInput(dnn::blobFromImage(crop_face, 1, Size(60, 60)));
            Mat raw_preds_landmarks =  net_landmarks.forward();
            raw_preds_landmarks *= crop_face.rows;
            const float* pred = raw_preds_landmarks.ptr<float>(0);

            Point face_left_top(pred[38], pred[39]);
            Point face_right_top(pred[66], pred[67]);
            if (pred[0] < 0.00001 && pred[1] < 0.0001||face_left_top.x - face_right_top.x >= 0) {
                error_cnt = 30;
                continue;
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
            Mat mask = Mat::zeros(crop_face.size(), CV_8U);
            cv::fillPoly(mask, pface1, 255);
            cv::fillPoly(mask, pface2, 255);
            emit signalReady(Scalar(mean(crop_face, mask)), color_mat_ts);

        }
        else {
            last_tracking_status = false;
            emit updateFrame(color_mat);
        }
    }
    if (use_camera) {
        cam_cap.release();
    }
    else {
        sensor->stop();
        rs2::frame f;
        fq.try_wait_for_frame(&f, 1);
    }
}


void Capture::initInferenceEngine() {
    // command of downloading from open_model_zoo: 
    // omz_downloader  --name face-detection-retail-0005 --output_dir C:\Users\b39b3\Documents\src\opencv\modules --precisions FP16,FP16-INT8,FP32
    string root_path = PROJECT_ROOT_PATH;
    string path_net_facedetect = root_path + "modules/intel/face-detection-retail-0005/FP32/face-detection-retail-0005";
    string path_net_landmarks = root_path + "modules/intel/facial-landmarks-35-adas-0002/FP32/facial-landmarks-35-adas-0002";
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
    net_landmarks.forward();

    sync_stage1 = cv::imread(PROJECT_ROOT_PATH"resources/sync_stage1.png");
    sync_stage2 = cv::imread(PROJECT_ROOT_PATH"resources/2stage_sync.png");
}