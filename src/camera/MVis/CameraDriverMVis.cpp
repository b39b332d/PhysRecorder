#include "CameraDriverMVis.h"

#include <codecvt>
#include <iostream>

namespace capture {


    CameraDeviceMVis::CameraDeviceMVis(tSdkCameraDevInfo* pcam_info)
    {
        cam_info = *pcam_info;
        device_name = "MV: "+std::string(pcam_info->acProductName) + " @" + std::string(pcam_info->acSn, 5);
    }

    bool CameraDeviceMVis::native_init()
    {
        CameraSdkStatus status;
        if ((status = CameraInit(&cam_info, -1, -1, &cam_device)) != CAMERA_STATUS_SUCCESS)
        {
            return false;
        }
        get_all_option_range_native();
        return true;
    }
    void CameraDeviceMVis::camera_read_loop() {
        tSdkFrameHead 	sFrameInfo;
        uint8_t* pbyBuffer;
        CameraSdkStatus cam_status;

        while (status != CameraDevice::CS_ON_STOP)
        {

            if (CameraGetImageBufferPriority(cam_device, &sFrameInfo, &pbyBuffer, 1000,
                CAMERA_GET_IMAGE_PRIORITY_NEWEST) == CAMERA_STATUS_SUCCESS)
            {

                auto frame = CameraAlignMalloc(m_sCameraInfo.sResolutionRange.iWidthMax * m_sCameraInfo.sResolutionRange.iHeightMax * 4, 16);
                // The original data obtained will be converted into RGB format data, and meanwhile through the ISP module, the image will be processed with noise reduction, edge enhancement and color correction.
                // Most of our company's cameras, the original data are Bayer format
                cam_status = CameraImageProcess(cam_device, pbyBuffer, frame, &sFrameInfo);

                if (cam_status == CAMERA_STATUS_SUCCESS)
                {
                    long long current_ts = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                    if (since == -1) since = current_ts- sFrameInfo.uiTimeStamp * 100;
                    only_stream->write(
                        only_stream->selected_profile->createFrame(
                            sFrameInfo.uiTimeStamp * 100 + since, (unsigned char*)frame,
                            sFrameInfo.uBytes,
                            [frame]() {
                                CameraAlignFree(frame);
                            }));
                }

                // After successfully calling CameraGetImageBuffer, you have to call CameraReleaseImageBuffer to release the obtained buffer.
                // Otherwise call CameraGetImageBuffer, the program will be suspended, that other threads call CameraReleaseImageBuffer to release the buffer
                //"Release the buffer which get from CameraSnapToBuffer or CameraGetImageBuffer"
                CameraReleaseImageBuffer(cam_device, pbyBuffer);
            }

        }
        onDeviceReadingFailed();
    }
    static std::set<CameraDeviceMVis*>running_cams;
    static void MVIS_stop() {
        for (auto cam : running_cams) {
            cam->status = CameraDevice::CS_ON_STOP;
            cam->camera_read_thread->join();
            CameraStop(cam->cam_device);
            CameraUnInit(cam->cam_device);
        }
    }
    bool CameraDeviceMVis::native_start()
    {
        if (CameraSetFrameSpeed(cam_device, 2) != CAMERA_STATUS_SUCCESS)
        return false;
        since = -1;
        if (CameraSetImageResolution(cam_device, &((CameraProfileMVis*)(only_stream->selected_profile))->resol) != CAMERA_STATUS_SUCCESS)
        return false;
        camera_read_thread = new std::thread(&CameraDeviceMVis::camera_read_loop,this);
        if (CameraPlay(cam_device) != CAMERA_STATUS_SUCCESS)
            return false;
        running_cams.insert(this);
        return true;
    }


    void CameraDeviceMVis::native_stop() {
        running_cams.erase(this);
        camera_read_thread->join();
        delete camera_read_thread;
        CameraStop(cam_device);
    }
    void CameraDeviceMVis::native_release() {
        CameraUnInit(cam_device);
    }
    CameraDeviceMVis::~CameraDeviceMVis()
    {
    }
    void CameraDeviceMVis::get_all_option_range_native() {
        CameraGetCapability(cam_device,&m_sCameraInfo);
        CameraLoadParameter(cam_device, PARAMETER_TEAM_DEFAULT);


        only_stream = new CameraStreamMVis(this);
        streams_map["Video"] = only_stream;
        only_stream->stream_name = "Video";
        CameraProfile* first_profile= nullptr;
        for (int i = 0; i < m_sCameraInfo.iImageSizeDesc; i++) {
            CameraProfile* p= new CameraProfileMVis(&m_sCameraInfo.pImageSizeDesc[i], only_stream);
            only_stream->profiles_map["BGR8"].insert(p);
            if (first_profile == nullptr)
                first_profile = p;
        }
        only_stream->default_profile = first_profile;

        for (int i = 0; i < DEVICE_OPTION_CNT; i++) {
            switch ((DEVICE_OPTION)i) {
            case CameraDevice::DEVICE_EXPOSURE:
            {
                double line_time, exp_time;
                CameraGetExposureLineTime(cam_device, &line_time);
                CameraGetExposureTime(cam_device, &exp_time);
                int num_line = exp_time / line_time;

                option_range opt_range = { 0, };
                opt_range.min = m_sCameraInfo.sExposeDesc.uiExposeTimeMin;
                opt_range.max = m_sCameraInfo.sExposeDesc.uiExposeTimeMax;
                opt_range.is_supported = true;
                opt_range.step = 1;
                opt_range.def.value = num_line;
                if (m_sCameraInfo.sIspCapacity.bAutoExposure) {
                    int is_auto_exp;
                    CameraGetAeState(cam_device, &is_auto_exp);
                    if (is_auto_exp) {
                        opt_range.def.status_type = OPTION_AUTO;
                        opt_range.current.status_type = OPTION_AUTO;
                    }
                    else {
                        opt_range.def.status_type = OPTION_MANUAL;
                        opt_range.current.status_type = OPTION_MANUAL;
                    }
                    opt_range.support_type = OPTION_AUTO;
                }
                else {
                    opt_range.def.status_type = OPTION_MANUAL;
                    opt_range.current.status_type = OPTION_MANUAL;
                    opt_range.support_type = OPTION_MANUAL;
                }
                opt_range.current.value = num_line;
                configurations[i] = opt_range;
            }
                break;;
            case CameraDevice::DEVICE_WHITE_BALANCE:

                break;
            case CameraDevice::DEVICE_GAIN:
                break;
            case CameraDevice::DEVICE_LIGHT:
                break;
            case CameraDevice::DEVICE_GAMMA:
                break;
            case DEVICE_CONTRAST:
                break;
            case DEVICE_HUE:
                break;
            case DEVICE_SATURATION:
                break;
            case DEVICE_SHARPNESS:
                break;
            case DEVICE_BACKLIGHT:
                break;
            case DEVICE_BRIGHTNESS:
                break;

            }
        }
    }

    void CameraDeviceMVis::set_option_native(DEVICE_OPTION option, const option_status& value)
    {
        switch (option) {
        case CameraDevice::DEVICE_EXPOSURE:
            if (value.status_type == OPTION_AUTO) {
                CameraSetAeState(cam_device, 1);
            }
            else {
                CameraSetAeState(cam_device, 0);
                double line_time;
                CameraGetExposureLineTime(cam_device, &line_time);
                CameraSetExposureTime(cam_device, line_time * value.value);
            }
            break;
        }
    }


    CameraStreamMVis::CameraStreamMVis( CameraDevice* device):
        CameraStream(device)
    {
    }

    CameraProfileMVis::CameraProfileMVis(tSdkImageResolution* p_resol,CameraStream* stream) :
         CameraProfile(stream), resol(*p_resol)
    {
        format = PIX_TYPE_BGR8;
        resolution.width = resol.iWidth;
        resolution.height = resol.iHeight;
        ratio.numerator = 300;
        ratio.denominator = 1;
    }

    CameraProfileMVis::~CameraProfileMVis()
    {
    }
    static bool mvis_init = false;
    std::vector<CameraDevice*> EnumerateCamera_MVis() {
        std::vector<CameraDevice*> devices;
        if (!mvis_init) {
            auto status = CameraSdkInit(0);
            std::atexit(MVIS_stop);
            mvis_init = true;
        }
        tSdkCameraDevInfo sCameraList[10];
        int iCameraNums = 10;
        if (CameraEnumerateDevice(sCameraList, &iCameraNums) != CAMERA_STATUS_SUCCESS)
        {
            return devices;
        }
        else {
            for (int i = 0; i < iCameraNums; i++) {
                devices.push_back(new CameraDeviceMVis(&sCameraList[i]));
            }
        }

        return devices;
    }
};