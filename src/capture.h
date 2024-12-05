#ifndef CAPTURE_H
#define CAPTURE_H
#include <QThread >
#include <string>
#include <vector>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>
#include <atomic>
#include <MediaWriter.h>
#include <CameraCapture.h>



extern std::mutex recorder_lock;
extern std::string record_prefix;
extern bool is_recording;
extern std::unordered_map<capture::CameraStream*, MediaWriter*> rec_maps;

class Capture : public QThread
{
    Q_OBJECT

    cv::dnn::Net net_face;
    cv::dnn::Net net_landmarks;

public:

    typedef enum {
        NOT_TRACKING,
        TRACKING_FACE,
        STATIC_ROI,
        TRACKING_LOSE
    } TRACKING_MODE;
    std::mutex track_lock;
    TRACKING_MODE tracking_mode;
    cv::Rect2i roi;

    int rot = 0;
    bool is_fliplr = false;
    bool is_flipud = false;
    capture::CameraDevice* selected_device = nullptr;
    Capture();

    Q_SIGNAL void signalReady(cv::Scalar, double);
    Q_SIGNAL void loseTracking();
    Q_SIGNAL void updateFrame(QList<RawFrame*> main_frames, QList<RawFrame*> other_frames, cv::Rect2i);
    Q_SIGNAL void device_disabled(capture::CameraDevice*);

private:
    void run();
    void initInferenceEngine();

};



#endif // CAPTURE_H
