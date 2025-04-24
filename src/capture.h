#ifndef CAPTURE_H
#define CAPTURE_H
#include <string>
#include <vector>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>
#include <atomic>
#include <MediaWriter.h>
#include <CameraCapture.h>
#include <face_tracking.h>
#include <QObject>

class Capture : public QObject
{
    Q_OBJECT

    FaceTracking* face_tracking = nullptr;
    std::mutex tracking_lock;
    std::mutex recorder_lock;
    bool is_recording = false;
    std::unordered_map<capture::CameraStream*, MediaWriter*> rec_maps;
public:
    capture::CameraStream* selected_stream = nullptr;

    int rot = 0;
    double scale = 1.0;
    bool is_fliplr = false;
    bool is_flipud = false;
    capture::CameraStream* last_stream = nullptr;
    Capture(FaceTracking* face_tracking);

    Q_SIGNAL void updateFrame(RawFrame* main_frames, QList<RawFrame*> other_frames, cv::Rect2i);
    Q_SIGNAL void device_disabled(capture::CameraDevice*);

    void setInference(FaceTracking* face_tracking);
    void startRecord(bool is_start=true);
    void recordStream(capture::CameraStream*, const std::string &file_name);
private:
    std::thread* th;
    void run();

};



#endif // CAPTURE_H
