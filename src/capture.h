#ifndef CAPTURE_H
#define CAPTURE_H
#include "signalprocess.h"
#include <QThread >
#include <string>
#include <vector>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>
#include <atomic>
#include "converter.h"
#include <MediaWriter.h>
#include <CameraCapture.h>



extern std::mutex recorder_lock;
extern std::string record_prefix;
extern bool is_recording;
extern std::unordered_map<capture::CameraStream*, std::pair<MediaWriter*, std::vector<double>*>> rec_maps;

class Capture : public QThread
{
    Q_OBJECT
        cv::Mat m_frame;
    std::vector<double> timestamp;
    int timestamp_method;
    int capture_ready;
    int timestamp_line_count;

    cv::dnn::Net net_face;
    cv::dnn::Net net_landmarks;
    Converter& converter;
    std::atomic<bool> suicide = false;
    const double interp_cyc;
    double fps = 0;
    int total_frames = 0;
    cv::Size2i vid_size;
    MediaWriter* rec=NULL;
    std::vector<double> rec_ts;
    float rec_fps ;
    cv::Mat sync_stage1, sync_stage2, depthLUT;
    cv::Mat LUT_16_reinterpret_cast(cv::Mat mat, cv::Mat dst);
    int cam_idx;
    std::mutex cv_cap_lock;

public:

    SignalProcess* signalProcess;
    std::atomic<bool> tracking = false; // can only be set to false
    std::atomic<int> detect_mode;
    std::atomic<int> speed;
    std::atomic<bool> pause = false;
    std::atomic<int> position = -1;
    std::atomic<int> rot = 0;
    std::atomic<bool> is_fliplr = false;
    std::atomic<bool> is_flipud = false;
    std::atomic<int> quality = -1;
    capture::CameraDevice* selected_device = nullptr;
    Capture(Converter& converter, SignalProcess*);

    Q_SIGNAL void signalReady(cv::Scalar, double);
    Q_SIGNAL void loseTracking();
    Q_SIGNAL void updateFrame(QList<RawFrame*> main_frames, QList<RawFrame*> other_frames, cv::Rect2i);
    Q_SIGNAL void device_disabled(capture::CameraDevice*);
    int total_time = 0;
    void wait_for_rec_save();
    bool use_camera;

private:
    void run();
    void initInferenceEngine();

};



#endif // CAPTURE_H
