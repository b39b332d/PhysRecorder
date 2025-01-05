#include "CameraDriverMSMF.h"
#include <Mferror.h>
#include <iostream>
#include "CameraCapture.h"
#include <atlbase.h>
#include <atlcom.h>
#include <strmif.h>
#include <codecvt>
#ifndef CAMMSMF_IF_EQUAL_RETURN
#define CAMMSMF_IF_EQUAL_RETURN(val) if(MFVideoFormat_##val == subtype) return PIX_TYPE_##val
#endif
namespace capture {
    inline PIX_TYPE GUID_to_pixformat(GUID subtype) {
        if (MFVideoFormat_RGB24 == subtype) return PIX_TYPE_BGR8;
        if (MFVideoFormat_RGB32 == subtype) return PIX_TYPE_BGRA;
        if (MFVideoFormat_ARGB32 == subtype) return PIX_TYPE_ARGB;
        if (MFVideoFormat_RGB555 == subtype) return PIX_TYPE_RGB5;
        if (MFVideoFormat_RGB565 == subtype) return PIX_TYPE_RGB6;
        CAMMSMF_IF_EQUAL_RETURN(RGB8);
        CAMMSMF_IF_EQUAL_RETURN(L16);
        CAMMSMF_IF_EQUAL_RETURN(D16);
        CAMMSMF_IF_EQUAL_RETURN(L8);

        return static_cast<PIX_TYPE>(subtype.Data1);;
    }

    std::string ws2s(const std::wstring& wstr)
    {
        using convert_typeX = std::codecvt_utf8<wchar_t>;
        std::wstring_convert<convert_typeX, wchar_t> converterX;

        return converterX.to_bytes(wstr);
    }
    CameraProfileMSMF::CameraProfileMSMF(CameraStreamMSMF* stream, IMFMediaType* pType) :
        CameraProfile(stream), pType(pType), dwStreamIndex(stream->stream_idx)
    {
        GUID gtype;
        pType->GetGUID(MF_MT_SUBTYPE, &gtype);
        format = GUID_to_pixformat(gtype);
        MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &resolution.width, &resolution.height);
        MFGetAttributeRatio(pType, MF_MT_FRAME_RATE, &ratio.numerator, &ratio.denominator);

    }
    CameraProfileMSMF::~CameraProfileMSMF() {
        pType->Release();
    }


    CameraStreamMSMF::CameraStreamMSMF(CameraDeviceMSMF* pdevice, IMFSourceReader* pReader, DWORD stream_idx, IMFMediaType* default_native_profile) :
        CameraStream(pdevice), stream_idx(stream_idx)
    {
        auto default_profile = new CameraProfileMSMF(this, default_native_profile);
        if (!default_profile->is_valid()) {
            return;
            delete default_profile;
        }

        GUID majorType;
        HRESULT hr = S_OK;
        hr = default_native_profile->GetGUID(MF_MT_MAJOR_TYPE, &majorType);
        THROW_HR(hr, "GetGUID Error");
        if (majorType == MFMediaType_Video)
            stream_name = std::format("{}:{}", "Video", stream_idx);
        else
            return;
        //else if (majorType == MFMediaType_Audio)
            //stream_name = std::format("{}:{}", "Audio", stream_idx);
        //else        
            //stream_name = std::format("{}:{}", "Unknow", stream_idx);

        DWORD dwMediaTypeIndex = 0;

        while (SUCCEEDED(hr))
        {
            IMFMediaType* pType = NULL;
            hr = pReader->GetNativeMediaType(stream_idx, dwMediaTypeIndex, &pType);
            if (hr == MF_E_NO_MORE_TYPES)
            {
                hr = S_OK;
                break;
            }
            else if (SUCCEEDED(hr))
            {
                auto p = new CameraProfileMSMF(this, pType);
                if (!p->is_valid()) delete p; //filter here
                else {
                    if (default_profile != NULL && *default_profile == *p) {
                        delete default_profile;
                        default_profile = NULL;
                        this->default_profile = p;
                    }
                    auto& ps = profiles_map[GET_PIX_TYPE_NAME(p->format)];
                    if (ps.find(p) != ps.end()) {
                        delete p;
                    }
                    else
                        profiles_map[GET_PIX_TYPE_NAME(p->format)].insert(p);
                }
            }
            ++dwMediaTypeIndex;
        }
    }
    std::string getSecondColumn(const std::string& input) {
        size_t start = input.find('#');
        if (start != std::string::npos) {
            size_t end = input.find('&', start + 1);
            if (end != std::string::npos) {
                // Extract the substring between the first and second '#'
                return input.substr(start + 1, end - start - 1);
            }
        }
        return "";
    }

    CameraDeviceMSMF::CameraDeviceMSMF(IMFActivate* m_pDevices) :
        m_pDevices(m_pDevices) {

        HRESULT hr;
        WCHAR* pszName;
        hr = m_pDevices->GetAllocatedString(
            MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
            &pszName,
            NULL
        );
        THROW_HR(hr, "GetAllocatedString Error");
        std::string str = ws2s(pszName);

        hr = m_pDevices->GetAllocatedString(
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,//use device hardware id!
            &pszName,
            NULL
        );
        std::string str_sz = ws2s(pszName);
        device_name = "MF: " + str + " @" + getSecondColumn(str_sz);
        CoTaskMemFree(pszName);

    }
    void CameraDeviceMSMF::native_release() {

    }
    template<class controller>
    inline option_range get_option_spCamera(controller& spControl, long opt_id) {
        long min, max, step, def, control;
        long value, flags;
        option_range opt_range = { 0, };
        HRESULT hr;
        if (spControl == nullptr) return opt_range;
        hr = spControl->GetRange(opt_id, &min, &max, &step, &def, &control);
        if (FAILED(hr)) return opt_range;
        hr = spControl->Get(opt_id, &value, &flags);
        if (FAILED(hr)) return opt_range;
        if (min - max == 0) return opt_range;
        opt_range.current.value = value;
        opt_range.min = min;
        opt_range.max = max;
        opt_range.step = step;
        opt_range.def.value = def;
        opt_range.def.status_type = OPTION_MANUAL;
        opt_range.support_type = OPTION_MANUAL;
        if (flags == 0x01) {
            opt_range.def.status_type = OPTION_AUTO;
            opt_range.current.status_type = OPTION_AUTO;
        }
        else {
            opt_range.current.status_type = OPTION_MANUAL;
        }
        if (control == 0x03)
            opt_range.support_type = OPTION_AUTO;
        else if (control == 0x01)
            opt_range.def.status_type = OPTION_AUTO;
        opt_range.is_supported = true;
        return opt_range;
    }
    void CameraDeviceMSMF::get_all_option_range_native(IMFMediaSource* pSource)
    {
        HRESULT hr;
        hr = m_pDevices->ActivateObject(IID_PPV_ARGS(&pSource));
        CComQIPtr<IAMCameraControl> spCameraControl=nullptr;
        CComQIPtr<IAMVideoProcAmp> spVideoControl=nullptr;
        hr = pSource->QueryInterface(IID_IAMCameraControl, (void**)&spCameraControl);
        hr = pSource->QueryInterface(IID_IAMVideoProcAmp, (void**)&spVideoControl);
        for (int i = 0; i < DEVICE_OPTION_CNT; i++) {
            switch ((DEVICE_OPTION)i) {
            case DEVICE_EXPOSURE:
                configurations[i] = get_option_spCamera(spCameraControl, CameraControl_Exposure);
                break;
            case DEVICE_ZOOM:
                configurations[i] = get_option_spCamera(spCameraControl, CameraControl_Zoom);
                break;
            case DEVICE_PAN:
                configurations[i] = get_option_spCamera(spCameraControl, CameraControl_Pan);
                break;
            case DEVICE_TILT:
                configurations[i] = get_option_spCamera(spCameraControl, CameraControl_Tilt);
                break;
            case DEVICE_IRIS:
                configurations[i] = get_option_spCamera(spCameraControl, CameraControl_Iris);
                break;
            case DEVICE_FOCUS:
                configurations[i] = get_option_spCamera(spCameraControl, CameraControl_Focus);
                break;
            case DEVICE_WHITE_BALANCE:
                configurations[i] = get_option_spCamera(spVideoControl, VideoProcAmp_WhiteBalance);
                break;
            case DEVICE_SHARPNESS:
                configurations[i] = get_option_spCamera(spVideoControl, VideoProcAmp_Sharpness);
                break;
            case DEVICE_CONTRAST:
                configurations[i] = get_option_spCamera(spVideoControl, VideoProcAmp_Contrast);
                break;
            case DEVICE_HUE:
                configurations[i] = get_option_spCamera(spVideoControl, VideoProcAmp_Hue);
                break;
            case DEVICE_SATURATION:
                configurations[i] = get_option_spCamera(spVideoControl, VideoProcAmp_Saturation);
                break;
            case DEVICE_GAMMA:
                configurations[i] = get_option_spCamera(spVideoControl, VideoProcAmp_Gamma);
                break;
            case DEVICE_BACKLIGHT:
                configurations[i] = get_option_spCamera(spVideoControl, VideoProcAmp_BacklightCompensation);
                break;
            case DEVICE_GAIN:
                configurations[i] = get_option_spCamera(spVideoControl, VideoProcAmp_Gain);
                break;
            case DEVICE_BRIGHTNESS:
                configurations[i] = get_option_spCamera(spVideoControl, VideoProcAmp_Brightness);
                break;
            case DEVICE_COLOR_ENABLED:
                configurations[i] = get_option_spCamera(spVideoControl, VideoProcAmp_ColorEnable);
                break;
            }
        }
    }

    template<class controller>
    inline option_status CameraDeviceMSMF::get_signle_option_spCamera(controller& spControl, long opt_id) {
        long value, flags;
        HRESULT hr;
        option_status ret_option = { 0, };
        hr = pSource->QueryInterface(IID_PPV_ARGS(&spControl));
        if (FAILED(hr))return ret_option;
        hr = spControl->Get(opt_id, &value, &flags);
        if (FAILED(hr)) return ret_option;
        ret_option.value = value;
        if (flags == 0x01)
            ret_option.status_type = OPTION_AUTO;
        else
            ret_option.status_type = OPTION_MANUAL;
        return ret_option;
    }
    option_status CameraDeviceMSMF::get_option_native(DEVICE_OPTION option)
    {
        IAMCameraControl* spCameraControl;
        IAMVideoProcAmp* spVideoControl;
        switch (option) {
        case DEVICE_EXPOSURE:
            return get_signle_option_spCamera(spCameraControl,CameraControl_Exposure);
        case DEVICE_ZOOM:
            return get_signle_option_spCamera(spCameraControl, CameraControl_Zoom);
        case DEVICE_PAN:
            return get_signle_option_spCamera(spCameraControl, CameraControl_Pan);
        case DEVICE_TILT:
            return get_signle_option_spCamera(spCameraControl, CameraControl_Tilt);
        case DEVICE_IRIS:
            return get_signle_option_spCamera(spCameraControl, CameraControl_Iris);
        case DEVICE_FOCUS:
            return get_signle_option_spCamera(spCameraControl, CameraControl_Focus);
        case DEVICE_WHITE_BALANCE:
            return get_signle_option_spCamera(spVideoControl, VideoProcAmp_WhiteBalance);
        case DEVICE_SHARPNESS:
            return get_signle_option_spCamera(spVideoControl, VideoProcAmp_Sharpness);
        case DEVICE_CONTRAST:
            return get_signle_option_spCamera(spVideoControl, VideoProcAmp_Contrast);
        case DEVICE_HUE:
            return get_signle_option_spCamera(spVideoControl, VideoProcAmp_Hue);
        case DEVICE_SATURATION:
            return get_signle_option_spCamera(spVideoControl, VideoProcAmp_Saturation);
        case DEVICE_GAMMA:
            return get_signle_option_spCamera(spVideoControl, VideoProcAmp_Gamma);
        case DEVICE_BACKLIGHT:
            return get_signle_option_spCamera(spVideoControl, VideoProcAmp_BacklightCompensation);
        case DEVICE_GAIN:
            return get_signle_option_spCamera(spVideoControl, VideoProcAmp_Gain);
        case DEVICE_BRIGHTNESS:
            return get_signle_option_spCamera(spVideoControl, VideoProcAmp_Brightness);
        case DEVICE_COLOR_ENABLED:
            return get_signle_option_spCamera(spVideoControl, VideoProcAmp_ColorEnable);
        default:
            return { 0, };
        }
    }

    template<class controller>
    inline void CameraDeviceMSMF::set_signle_option_spCamera(controller& spControl, const long& opt_id, option_range&range, const option_status& option) {
        HRESULT hr;
        hr = pSource->QueryInterface(IID_PPV_ARGS(&spControl));
        if (FAILED(hr))return;
        if (option.status_type == OPTION_AUTO) {
            hr = spControl->Set(opt_id, range.def.value, 0x01);
            range.current.value = range.def.value;
            range.current.status_type = OPTION_AUTO;
        }
        else {
            hr = spControl->Set(opt_id, option.value, 0x02);
            range.current.value = option.value;
            range.current.status_type = OPTION_MANUAL;
        }
    }

    void CameraDeviceMSMF::set_option_native(DEVICE_OPTION option, const option_status& opt)
    {
        IAMCameraControl* spCameraControl;
        IAMVideoProcAmp* spVideoControl;
        switch (option) {
        case CameraDevice::DEVICE_EXPOSURE:
            set_signle_option_spCamera(spCameraControl, CameraControl_Exposure, configurations[option],opt);
            break;
        case DEVICE_ZOOM:
            set_signle_option_spCamera(spCameraControl, CameraControl_Zoom, configurations[option],opt);
            break;
        case DEVICE_PAN:
            set_signle_option_spCamera(spCameraControl, CameraControl_Pan, configurations[option],opt);
            break;
        case DEVICE_TILT:
            set_signle_option_spCamera(spCameraControl, CameraControl_Tilt, configurations[option],opt);
            break;
        case DEVICE_IRIS:
            set_signle_option_spCamera(spCameraControl, CameraControl_Iris, configurations[option],opt);
            break;
        case DEVICE_FOCUS:
            set_signle_option_spCamera(spCameraControl, CameraControl_Focus, configurations[option],opt);
            break;
        case DEVICE_WHITE_BALANCE:
            set_signle_option_spCamera(spVideoControl, VideoProcAmp_WhiteBalance, configurations[option],opt);
            break;
        case DEVICE_SHARPNESS:
            set_signle_option_spCamera(spVideoControl, VideoProcAmp_Sharpness, configurations[option],opt);
            break;
        case DEVICE_CONTRAST:
            set_signle_option_spCamera(spVideoControl, VideoProcAmp_Contrast, configurations[option],opt);
            break;
        case DEVICE_HUE:
            set_signle_option_spCamera(spVideoControl, VideoProcAmp_Hue, configurations[option],opt);
            break;
        case DEVICE_SATURATION:
            set_signle_option_spCamera(spVideoControl, VideoProcAmp_Saturation, configurations[option],opt);
            break;
        case DEVICE_GAMMA:
            set_signle_option_spCamera(spVideoControl, VideoProcAmp_Gamma, configurations[option],opt);
            break;
        case DEVICE_BACKLIGHT:
            set_signle_option_spCamera(spVideoControl, VideoProcAmp_BacklightCompensation, configurations[option],opt);
            break;
        case DEVICE_GAIN:
            set_signle_option_spCamera(spVideoControl, VideoProcAmp_Gain, configurations[option],opt);
            break;
        case DEVICE_BRIGHTNESS:
            set_signle_option_spCamera(spVideoControl, VideoProcAmp_Brightness, configurations[option],opt);
            break;
        case DEVICE_COLOR_ENABLED:
            set_signle_option_spCamera(spVideoControl, VideoProcAmp_ColorEnable, configurations[option], opt);
            break;
        }
        return;
    }
    bool CameraDeviceMSMF::native_init() {
        IMFMediaSource* pSource;
        IMFSourceReader* pReader;
        HRESULT hr;
        hr = m_pDevices->ActivateObject(IID_PPV_ARGS(&pSource));
        THROW_HR(hr, "ActivateObject Error");
        IMFAttributes* pAttributes = nullptr;
        hr = MFCreateAttributes(&pAttributes, 2);  // Create attributes with a capacity of 1
        THROW_HR(hr, "MFCreateAttributes Error");

        // Enable video processing (e.g., color conversion, scaling)
        hr = pAttributes->SetUINT32(MF_READWRITE_DISABLE_CONVERTERS, TRUE);

        if (FAILED(hr)) {
            pAttributes->Release();
            THROW_HR(hr, "SetUINT32 Error");
        }

        hr = MFCreateSourceReaderFromMediaSource(pSource, NULL, &pReader);
        THROW_HR(hr, "MFCreateSourceReaderFromMediaSource Error");

        DWORD dwStreamIndex = 0;

        while (SUCCEEDED(hr))
        {

            IMFMediaType* pType = NULL;
            hr = pReader->GetCurrentMediaType(dwStreamIndex, &pType);

            if (hr == MF_E_INVALIDSTREAMNUMBER)
            {
                hr = S_OK;
                break;
            }
            THROW_HR(hr, "GetCurrentMediaType Error");
            auto s = new CameraStreamMSMF(this, pReader, dwStreamIndex, pType);
            if (!s->is_valid()) delete s;
            else {
                streams_map[s->stream_name] = s;
            }

            ++dwStreamIndex;
        }
        hr = m_pDevices->DetachObject();
        THROW_HR(hr, "DetachObject Error");
        get_all_option_range_native(pSource);
        pReader->Release();
        pSource->Release(); // auto shutdown by pReader
        return true;
    }


    bool CameraDeviceMSMF::native_start()
    {
        HRESULT hr;
        IMFAttributes* pAttributes = nullptr;
        hr = m_pDevices->ActivateObject(IID_PPV_ARGS(&pSource));
        if (FAILED(hr)) goto native_start_failed1;
        cam_reader = new CameraReaderMSMF(this);

        hr = MFCreateAttributes(&pAttributes, 2);  // Create attributes with a capacity of 1
        if (FAILED(hr)) goto native_start_failed2;

        // Enable video processing (e.g., color conversion, scaling)
        hr = pAttributes->SetUINT32(MF_READWRITE_DISABLE_CONVERTERS, TRUE);
        hr = pAttributes->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, cam_reader);

        if (FAILED(hr)) {
            pAttributes->Release();
            goto native_start_failed2;
        }

        hr = MFCreateSourceReaderFromMediaSource(pSource, pAttributes, &pReader);

        pAttributes->Release();
        if (FAILED(hr)) goto native_start_failed2;
        cam_reader->pReader = pReader;

        for (auto s : enabled_streams) {
            pReader->SetCurrentMediaType(((CameraStreamMSMF*)s)->stream_idx,
                NULL, ((CameraProfileMSMF*)(s->selected_profile))->pType);
            pReader->SetStreamSelection(((CameraStreamMSMF*)s)->stream_idx, TRUE);
        }
        hr = pReader->ReadSample(MF_SOURCE_READER_ANY_STREAM, 0, NULL, NULL, NULL, NULL);

        if (FAILED(hr)) goto native_start_failed3;

        return true;

    native_start_failed3:
        pReader->Release();
        cam_reader->pReader = nullptr;
    native_start_failed2:
        pSource->Release();
    native_start_failed1:
        return false;
    }

    void CameraDeviceMSMF::native_stop()
    {
        HRESULT hr = S_OK;
        hr = m_pDevices->ShutdownObject();
        pReader->Release();
        pSource = nullptr;
        return;
    }

    static bool is_MFStartup_init = false;

    std::vector<CameraDevice*> EnumerateCamera_MSMF() {
        HRESULT hr;
        IMFAttributes* pAttributes = NULL;
        UINT32      m_cDevices; // contains the number of devices
        IMFActivate** m_ppDevices; // contains properties about each device
        std::vector<CameraDevice*> devices;
        if (!is_MFStartup_init) {
            CoInitializeEx(NULL, COINIT_MULTITHREADED);
            MFStartup(MF_VERSION); 
            is_MFStartup_init = true;
        }
        else {
            MFShutdown();
            MFStartup(MF_VERSION);
        }
        // Enumerate devices.
        hr = MFCreateAttributes(&pAttributes, 1);
        THROW_HR(hr, "MFCreateAttributes Error");

        // Ask for source type = video capture devices
        hr = pAttributes->SetGUID(
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
        );
        THROW_HR(hr, "SetGUID Error");
        // Enumerate devices.
        hr = MFEnumDeviceSources(pAttributes, &m_ppDevices, &m_cDevices);
        THROW_HR(hr, "MFEnumDeviceSources Error");

        pAttributes->Release();


        for (UINT32 iDevice = 0; iDevice < m_cDevices; iDevice++)
        {
            devices.push_back(new CameraDeviceMSMF(m_ppDevices[iDevice]));
        }
        CoTaskMemFree(m_ppDevices);

        return devices;
    }
};