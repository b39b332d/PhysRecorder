#ifndef CAPTURE_H
#define CAPTURE_H
#include "signalprocess.h"
#include <QThread >
#include <string>
#include <vector>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/dnn.hpp>
#include <atomic>
#include "converter.h"
#include <librealsense2/rs.hpp>
#include <opencv2/objdetect.hpp>
struct CameraInfo
{
    double fps;
    int width;
    int height;
    bool useCameraTimestamp;
};
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
    rs2::sensor* sensor;
    rs2::frame_queue fq;
    cv::Size2i vid_size;
    std::mutex recorder_lock;
    cv::VideoWriter* rec=NULL;
    std::vector<double> rec_ts;
    float rec_fps ;
    cv::QRCodeDetector qrDecoder;
    cv::Mat sync_stage1, sync_stage2, depthLUT;
    cv::Mat LUT_16_reinterpret_cast(cv::Mat mat, cv::Mat dst);
    int cam_idx;
    cv::VideoCapture cam_cap;
    std::mutex cv_cap_lock;

public:
    std::atomic<bool> isRecording = false;
    std::atomic<bool> isRunning = false;
    SignalProcess* signalProcess;
    std::atomic<bool> tracking = false; // can only be set to false
    std::atomic<int> detect_mode;
    std::atomic<int> speed;
    std::atomic<bool> pause = false;
    std::atomic<int> position = -1;
    std::atomic<int> rot = 0;
    std::atomic<bool> is_fliplr = false;
    std::atomic<bool> is_flipud = false;
    std::atomic<bool> is_syncing = false;
    Capture(Converter& converter, SignalProcess*);
    Q_SIGNAL void facesReady(const cv::Mat&, std::vector<cv::Rect>*);
    Q_SIGNAL void faceReady(const cv::Mat&, cv::Rect);
    Q_SIGNAL void signalReady(cv::Scalar, double);
    Q_SIGNAL void loseTracking();
    Q_SIGNAL void capInfoReady(float, float, float);
    Q_SIGNAL void updateFrame(const cv::Mat&);
    Q_SIGNAL void fpsReady(double,int,int);
    Q_SIGNAL void tsofsReady(double);
    Q_SIGNAL void cap_started();
    void setCapture(rs2::sensor& sensor,int,int,int);
    void setCVCamProperty(int propId, double value);
    void setCamera(int cam_idx,int width,int height,float fps);
    void stop();
    int total_time = 0;
    std::atomic<char*> save_path=NULL;
    void wait_for_rec_save();
    bool use_camera;

private:
    void run();
    void initInferenceEngine();

};



#endif // CAPTURE_H
