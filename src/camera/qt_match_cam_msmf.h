
#ifndef _QT_MATCH_CAM_MSMF_H_
#define _QT_MATCH_CAM_MSMF_H_
#include <QList>

typedef struct {
    int idx;
    QString name;
    QList<QSize> resolutions;
    QList<float> frameRate;
} CamInfo;

QList<CamInfo> get_camera_map();
#endif