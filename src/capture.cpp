#include "capture.h"
#include <iostream>
#include <vector>
#include <deque>
#include <qdebug>
#include "cnpy.h"
#include <QSharedPointer>
#include <algorithm>
#include <librealsense2/h/rs_types.h>
using namespace std;
using namespace cv;
using namespace cnpy;
using namespace cv::dnn;



Capture::Capture(Converter& converter, SignalProcess* sp) :
    converter(converter),
    tracking(true),
    detect_mode(0), // 0->manual 1->center 2-> auto
    cap(),
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
}


void Capture::setCapture(rs2::sensor& sensor,int width,int height,int fps) {
    this->sensor = &sensor;
    this->vid_size = Size2i(width, height);
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
    recorder_lock.lock();
    (*rec).release();
    delete rec;
    rec = NULL;
    string fname = "./rec/";
    cnpy::npy_save(fname+ string(save_path) + "vid_ts.npy", rec_ts);
    rec_ts.clear();
    recorder_lock.unlock();
}
void Capture::run()
{
    int i_loop = 0;
    double last_ts = 0;
    double fps = 0;
    int rot_prev = 0;
    cv::Size2i vid_size_rot = vid_size;
    cv::Mat rot_mat;
    Rect2i* rect_face=NULL;
    sensor->start(fq);
    int error_cnt = 0;
    static bool last_tracking_status = true;
    emit loseTracking();
    while (true)
    {

        if (isRunning == false) {
            break;
        }
        rs2::frame f = fq.wait_for_frame();
        auto color_mat = Mat(this->vid_size, CV_8UC3, (void*)f.get_data(), Mat::AUTO_STEP);
        double color_mat_ts = f.get_timestamp() / 1000.0;

        i_loop++;
        if (i_loop % 10 == 0) {
            fps = 10.0 / (color_mat_ts - last_ts);
            last_ts = color_mat_ts;
            emit fpsReady(fps);
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
    sensor->stop();
    rs2::frame f;
    fq.try_wait_for_frame(&f,1);
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
}