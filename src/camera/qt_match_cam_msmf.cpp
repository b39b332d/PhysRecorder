
#include <QCamera>
#include <QMediaDevices>

#include <detect_cam.h>
#include <qt_match_cam_msmf.h>

QList<CamInfo> get_camera_map() {
    std::shared_ptr<std::vector<WCHAR*>> msmf_cameras_ids = GetDeviceList();
    QList<CamInfo> msmf_cameras;

    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    int n_cams = cameras.size();

    for (const QCameraDevice& cameraDevice : cameras) {
        auto cam_id = cameraDevice.id();
        int found_idx = 0;
        for (auto msmf_cameras_id : *msmf_cameras_ids) {
            if (QString::fromWCharArray(msmf_cameras_id) == cam_id) {
                break;
            }
            found_idx++;
        }
        if (found_idx == msmf_cameras_ids->size()) {
            continue;
        }

        QList<QSize> resolutions;
        QList<float> frameRate;
        for (auto format : cameraDevice.videoFormats()) {
            int i_r = 0;
            for (auto resol : resolutions) {
                if (resol == format.resolution()) {
                    if (frameRate[i_r] < format.maxFrameRate())
                        frameRate[i_r] = format.maxFrameRate();
                    i_r = -1;
                    break;
                }
                i_r++;
            }
            if (i_r != -1) {
                resolutions.push_back(format.resolution());
                frameRate.push_back(format.maxFrameRate());
            }
        }
        msmf_cameras.append({ found_idx ,cameraDevice.description(), resolutions ,frameRate });

    }
    return msmf_cameras;
}