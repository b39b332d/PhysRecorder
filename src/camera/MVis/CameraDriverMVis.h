#ifndef _CAMERADRIVER_MVIS_H_
#define _CAMERADRIVER_MVIS_H_

#include <string>
#include <CameraDriver.h>

#include <windows.h>
#include "CameraApi.h"
namespace capture {
    class CameraStreamMVis;
    class CameraProfileMVis :public CameraProfile {
    public:

        std::string stream_name;
        tSdkImageResolution resol;
        CameraProfileMVis(tSdkImageResolution* resol,CameraStream* stream);
        ~CameraProfileMVis();
    };

    class CameraDeviceMVis;
    class CameraStreamMVis :public CameraStream {
    public:
        int stream_index;
        CameraStreamMVis(const std::string& stream_name,CameraDevice* device);

    };
    class CameraDeviceMVis :public CameraDevice {
        CameraHandle cam_device;
        tSdkCameraDevInfo cam_info;

        tSdkCameraCapbility m_sCameraInfo;
        std::thread *camera_read_thread=NULL;
        CameraStream* only_stream;
        long long since = -1;
    public:
        CameraDeviceMVis(tSdkCameraDevInfo* pcam_info);
        bool native_init();
        bool native_start();
        void native_stop();
        void native_release();

        ~CameraDeviceMVis();

         void get_all_option_range_native();
         void set_option_native(DEVICE_OPTION option, const option_status& value);
    private :
        void camera_read_loop();
        friend void MVIS_stop();
    };


    std::vector<CameraDevice*> EnumerateCamera_MVis();
}
#endif