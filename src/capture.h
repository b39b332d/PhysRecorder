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


extern std::mutex recorder_lock;
extern std::string record_prefix;
extern bool is_recording;
extern std::unordered_map<capture::CameraStream*, MediaWriter*> rec_maps;

class Capture : public QObject
{
    Q_OBJECT

    FaceTracking *face_tracking;
public:

    int rot = 0;
    bool is_fliplr = false;
    bool is_flipud = false;
    capture::CameraStream* selected_stream = nullptr;
    Capture(FaceTracking* face_tracking);

    Q_SIGNAL void updateFrame(QList<RawFrame*> main_frames, QList<RawFrame*> other_frames, cv::Rect2i);
    Q_SIGNAL void device_disabled(capture::CameraDevice*);

private:
    std::thread* th;
    void run();

};



#endif // CAPTURE_H
