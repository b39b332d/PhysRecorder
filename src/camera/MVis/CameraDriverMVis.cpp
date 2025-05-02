#include "CameraDriverMVis.h"

#include <codecvt>
#include <chrono>
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
                        only_stream->get_current_profile()->createFrame(
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
        if (CameraSetImageResolution(cam_device, &((CameraProfileMVis*)(only_stream->get_current_profile()))->resol) != CAMERA_STATUS_SUCCESS)
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


        only_stream = new CameraStreamMVis("Video",this);
        streams_map["Video"] = only_stream;
        CameraProfile* first_profile= nullptr;
        for (int i = 0; i < m_sCameraInfo.iImageSizeDesc; i++) {
            CameraProfile* p= new CameraProfileMVis(&m_sCameraInfo.pImageSizeDesc[i], only_stream);
            only_stream->profiles_map["BGR8"].insert(p);
            if (first_profile == nullptr)
                first_profile = p;
        }
        only_stream->default_profile = first_profile;
        int err;
        for (int i = 0; i < DEVICE_OPTION_CNT; i++) {
            option_range opt_range = { 0, };
            switch ((DEVICE_OPTION)i) {
            case DEVICE_EXPOSURE:
            {
                double line_time, exp_time;
                err = CameraGetExposureLineTime(cam_device, &line_time);
                if (err != CAMERA_STATUS_SUCCESS)line_time = 1;
                err = CameraGetExposureTime(cam_device, &exp_time);
                if (err != CAMERA_STATUS_SUCCESS)break;
                int num_line = exp_time / line_time;
                opt_range.min = m_sCameraInfo.sExposeDesc.uiExposeTimeMin;
                opt_range.max = m_sCameraInfo.sExposeDesc.uiExposeTimeMax;
                opt_range.is_supported = true;
                opt_range.step = 1;
                opt_range.scaled_factor = 1000.0/line_time;
                opt_range.def.value = num_line;
                if (m_sCameraInfo.sIspCapacity.bAutoExposure) {
                    int is_auto_exp = 0;
                    err = CameraGetAeState(cam_device, &is_auto_exp);
                    if (err != CAMERA_STATUS_SUCCESS) {
                        opt_range.def.status_type = OPTION_MANUAL;
                        opt_range.current.status_type = OPTION_MANUAL;
                        opt_range.support_type = OPTION_MANUAL;

                    }
                    else {
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
                }
                else {
                    opt_range.def.status_type = OPTION_MANUAL;
                    opt_range.current.status_type = OPTION_MANUAL;
                    opt_range.support_type = OPTION_MANUAL;
                }
                opt_range.current.value = num_line;
            }
                break;;
            case DEVICE_WHITE_BALANCE:
                if (m_sCameraInfo.iClrTempDesc <= 0)break;
                opt_range.min = 0;
                opt_range.max = m_sCameraInfo.iClrTempDesc;
                opt_range.step = 1;
                opt_range.scaled_factor = 1;
                err = CameraGetPresetClrTemp(cam_device, &opt_range.def.value);
                if (err != CAMERA_STATUS_SUCCESS) break;
                err = CameraGetClrTempMode(cam_device, &opt_range.current.value);
                if (err != CAMERA_STATUS_SUCCESS) break;
                opt_range.def.status_type = OPTION_MANUAL;
                opt_range.current.status_type = OPTION_MANUAL;
                opt_range.support_type = OPTION_MANUAL;
                opt_range.is_supported = true;
                break;
            case DEVICE_GAIN:
                opt_range.min = m_sCameraInfo.sExposeDesc.uiAnalogGainMin;
                opt_range.max = m_sCameraInfo.sExposeDesc.uiAnalogGainMax;
                if(opt_range.max - opt_range.min <= 0)break;
                opt_range.scaled_factor = 1.0 / m_sCameraInfo.sExposeDesc.fAnalogGainStep;
                opt_range.step = 1;
                err = CameraGetAnalogGain(cam_device, &opt_range.current.value);
                if (err != CAMERA_STATUS_SUCCESS) break;
                opt_range.def.value = opt_range.current.value;
                opt_range.def.status_type = OPTION_MANUAL;
                opt_range.current.status_type = OPTION_MANUAL;
                opt_range.support_type = OPTION_MANUAL;
                opt_range.is_supported = true;
                break;
            case DEVICE_LIGHT:
                int mode;
                err = CameraGetStrobeMode(cam_device, &mode);
                if (err != CAMERA_STATUS_SUCCESS) break;
                if (mode == STROBE_SYNC_WITH_TRIG_AUTO) {
                    opt_range.current.status_type = OPTION_AUTO;
                    opt_range.def.value = STROBE_SYNC_WITH_TRIG_MANUAL;
                }
                else {
                    opt_range.current.status_type = OPTION_MANUAL;
                    opt_range.def.value = mode;
                }
                opt_range.def.status_type = opt_range.current.status_type;
                opt_range.is_supported = true;
                break;
            case DEVICE_GAMMA:
                opt_range.min = m_sCameraInfo.sGammaRange.iMin;
                opt_range.max = m_sCameraInfo.sGammaRange.iMax;
                if (opt_range.max - opt_range.min <= 0)break;
                opt_range.scaled_factor = 1;
                opt_range.step = 1;
                err = CameraGetGamma(cam_device, &opt_range.current.value);
                if (err != CAMERA_STATUS_SUCCESS) break;
                opt_range.def.value = opt_range.current.value;
                opt_range.def.status_type = OPTION_MANUAL;
                opt_range.current.status_type = OPTION_MANUAL;
                opt_range.support_type = OPTION_MANUAL;
                opt_range.is_supported = true;
                break;
            case DEVICE_CONTRAST:
                opt_range.min = m_sCameraInfo.sContrastRange.iMin;
                opt_range.max = m_sCameraInfo.sContrastRange.iMax;
                if (opt_range.max - opt_range.min <= 0)break;
                opt_range.scaled_factor = 1;
                opt_range.step = 1;
                err = CameraGetContrast(cam_device, &opt_range.current.value);
                if (err != CAMERA_STATUS_SUCCESS) break;
                opt_range.def.value = opt_range.current.value;
                opt_range.def.status_type = OPTION_MANUAL;
                opt_range.current.status_type = OPTION_MANUAL;
                opt_range.support_type = OPTION_MANUAL;
                opt_range.is_supported = true;
                break;
            case DEVICE_SATURATION:
                opt_range.min = m_sCameraInfo.sSaturationRange.iMin;
                opt_range.max = m_sCameraInfo.sSaturationRange.iMax;
                if (opt_range.max - opt_range.min <= 0)break;
                opt_range.scaled_factor = 1;
                opt_range.step = 1;
                err = CameraGetSaturation(cam_device, &opt_range.current.value);
                if (err != CAMERA_STATUS_SUCCESS) break;
                opt_range.def.value = opt_range.current.value;
                opt_range.def.status_type = OPTION_MANUAL;
                opt_range.current.status_type = OPTION_MANUAL;
                opt_range.support_type = OPTION_MANUAL;
                opt_range.is_supported = true;
                break;
            case DEVICE_SHARPNESS:
                opt_range.min = m_sCameraInfo.sSharpnessRange.iMin;
                opt_range.max = m_sCameraInfo.sSharpnessRange.iMax;
                if (opt_range.max - opt_range.min <= 0)break;
                opt_range.scaled_factor = 1;
                opt_range.step = 1;
                err = CameraGetSharpness(cam_device, &opt_range.current.value);
                if (err != CAMERA_STATUS_SUCCESS) break;
                opt_range.def.value = opt_range.current.value;
                opt_range.def.status_type = OPTION_MANUAL;
                opt_range.current.status_type = OPTION_MANUAL;
                opt_range.support_type = OPTION_MANUAL;
                opt_range.is_supported = true;
                break;
            case DEVICE_BRIGHTNESS:
                opt_range.min = m_sCameraInfo.sExposeDesc.uiTargetMin;
                opt_range.max = m_sCameraInfo.sExposeDesc.uiTargetMax;
                if (opt_range.max - opt_range.min <= 0)break;
                opt_range.scaled_factor = 1;
                opt_range.step = 1;
                err = CameraGetAeTarget(cam_device, &opt_range.current.value);
                if (err != CAMERA_STATUS_SUCCESS) break;
                opt_range.def.value = opt_range.current.value;
                opt_range.def.status_type = OPTION_MANUAL;
                opt_range.current.status_type = OPTION_MANUAL;
                opt_range.support_type = OPTION_MANUAL;
                opt_range.is_supported = true;
                break;

            }
            set_option_range(i, opt_range);
        }
    }

    void CameraDeviceMVis::set_option_native(int option, const option_status& value)
    {
        int err;
        switch (option) {
        case DEVICE_EXPOSURE:
            if (configurations[option].is_supported && value.status_type != configurations[option].current.status_type) {
                err = CameraSetAeState(cam_device, value.status_type == OPTION_AUTO? 1:0);
                if(err == 0)
                    configurations[option].current.status_type = value.status_type;
            }
            if(configurations[option].is_supported &&  value.status_type != OPTION_AUTO) {
                err = CameraSetExposureTime(cam_device, (double)value.value/ configurations[option].scaled_factor*1000);
                if (err == 0)
                    configurations[option].current.value = value.value;
            }
            break;
        case DEVICE_WHITE_BALANCE:
            if (configurations[option].is_supported) {
                err = CameraSetClrTempMode(cam_device, value.value);
                if (err == 0)
                    configurations[option].current.value = value.value;
            }
            break;
        case DEVICE_GAIN:
            if (configurations[option].is_supported) {
                err = CameraSetAnalogGain(cam_device, value.value);
                if (err == 0)
                    configurations[option].current.value = value.value;
            }
            break;
        case DEVICE_LIGHT:
            if (configurations[option].is_supported) {
                if (value.status_type == OPTION_AUTO) {
                    err = CameraSetStrobeMode(cam_device, STROBE_SYNC_WITH_TRIG_AUTO);
                }
                else {
                    err = CameraSetStrobeMode(cam_device, configurations[option].def.value);
                }
                if (err == 0)
                    configurations[option].current.value = value.value;
            }
            break;
        case DEVICE_GAMMA:
            if (configurations[option].is_supported) {
                err = CameraSetGamma(cam_device, value.value);
                if (err == 0)
                    configurations[option].current.value = value.value;
            }
            break;
        case DEVICE_CONTRAST:
            if (configurations[option].is_supported) {
                err = CameraSetContrast(cam_device, value.value);
                if (err == 0)
                    configurations[option].current.value = value.value;
            }
            break;
        case DEVICE_SATURATION:
            if (configurations[option].is_supported) {
                err = CameraSetSaturation(cam_device, value.value);
                if (err == 0)
                    configurations[option].current.value = value.value;
            }
            break;
        case DEVICE_SHARPNESS:
            if (configurations[option].is_supported) {
                err = CameraSetSharpness(cam_device, value.value);
                if (err == 0)
                    configurations[option].current.value = value.value;
            }
            break;
        case DEVICE_BRIGHTNESS:
            if (configurations[option].is_supported) {
                err = CameraSetAeTarget(cam_device, value.value);
                if (err == 0)
                    configurations[option].current.value = value.value;
            }
            break;
        }
    }


    CameraStreamMVis::CameraStreamMVis(const std::string& stream_name, CameraDevice* device):
        CameraStream(stream_name,device)
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