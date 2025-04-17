#pragma once
#include <QObject>
#include <face_tracking.h>
class ColorExtractor : public QObject, public FaceTracking {
    Q_OBJECT
public:
    ColorExtractor(Inference* inf_backend) :FaceTracking(inf_backend) {
    };
    ~ColorExtractor() {
    };
    Q_SIGNAL void on_face_lost();
    Q_SIGNAL void on_signal_ready(FaceRoi* signal);
};