#include <iostream>
#include <librealsense2/rs.hpp>

using namespace std;
using namespace rs2;

const char* rs2_option_enum_name(int i);

int main()
{
    rs2::config rscfg;
    rscfg.enable_stream(RS2_STREAM_COLOR, 1280, 720, RS2_FORMAT_BGR8, 30);
    rscfg.enable_stream(RS2_STREAM_INFRARED, 1280, 720, RS2_FORMAT_Y8, 30);
    rscfg.enable_stream(RS2_STREAM_DEPTH, 1280, 720, RS2_FORMAT_Z16, 30);
    pipeline pipe;
    pipeline_profile selection = pipe.start(rscfg);
    auto sensors = selection.get_device().query_sensors();
    for (auto sr : sensors)
    {
        cout << "sensor:" << sr.get_info(RS2_CAMERA_INFO_NAME) << endl;
        for (int i = 0; i < 48; i++)
        {
            if (sr.supports((rs2_option)i))
            {
                option_range ora = sr.get_option_range((rs2_option)i);
                cout << rs2_option_enum_name(i) << ":\t" << rs2_option_to_string((rs2_option)i) << " Range: \tMin:" << ora.min << " Max:" << ora.max << " Step:" << ora.step << " Default:" << ora.def;
                bool optro;
                if (optro = sr.is_option_read_only((rs2_option)i))
                {
                    cout << " ro" << endl;
                }
                else
                {
                    cout << " rw" << endl;
                }

                if ((ora.max - ora.min) / ora.step < 20 && (!optro))
                {
                    cout << "--------------------------------------" << endl;
                    for (float val = ora.min; val <= ora.max; val += ora.step)
                    {
                        if (sr.get_option_value_description((rs2_option)i, val))
                            cout << val << ": " << sr.get_option_value_description((rs2_option)i, val) << endl;
                    }
                    cout << "--------------------------------------" << endl;
                }
            }
        }
    }

    for (auto& sensor : sensors) {
        std::vector<rs2::stream_profile> stream_profiles = sensor.get_stream_profiles();
    }

    getchar();
}

const char* rs2_option_enum_name(int i)
{
    switch (i)
    {
    case (int)RS2_OPTION_BACKLIGHT_COMPENSATION: return "RS2_OPTION_BACKLIGHT_COMPENSATION";
    case (int)RS2_OPTION_BRIGHTNESS: return "RS2_OPTION_BRIGHTNESS";
    case (int)RS2_OPTION_CONTRAST: return "RS2_OPTION_CONTRAST";
    case (int)RS2_OPTION_EXPOSURE: return "RS2_OPTION_EXPOSURE";
    case (int)RS2_OPTION_GAIN: return "RS2_OPTION_GAIN";
    case (int)RS2_OPTION_GAMMA: return "RS2_OPTION_GAMMA";
    case (int)RS2_OPTION_HUE: return "RS2_OPTION_HUE";
    case (int)RS2_OPTION_SATURATION: return "RS2_OPTION_SATURATION";
    case (int)RS2_OPTION_SHARPNESS: return "RS2_OPTION_SHARPNESS";
    case (int)RS2_OPTION_WHITE_BALANCE: return "RS2_OPTION_WHITE_BALANCE";
    case (int)RS2_OPTION_ENABLE_AUTO_EXPOSURE: return "RS2_OPTION_ENABLE_AUTO_EXPOSURE";
    case (int)RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE: return "RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE";
    case (int)RS2_OPTION_VISUAL_PRESET: return "RS2_OPTION_VISUAL_PRESET";
    case (int)RS2_OPTION_LASER_POWER: return "RS2_OPTION_LASER_POWER";
    case (int)RS2_OPTION_ACCURACY: return "RS2_OPTION_ACCURACY";
    case (int)RS2_OPTION_MOTION_RANGE: return "RS2_OPTION_MOTION_RANGE";
    case (int)RS2_OPTION_FILTER_OPTION: return "RS2_OPTION_FILTER_OPTION";
    case (int)RS2_OPTION_CONFIDENCE_THRESHOLD: return "RS2_OPTION_CONFIDENCE_THRESHOLD";
    case (int)RS2_OPTION_EMITTER_ENABLED: return "RS2_OPTION_EMITTER_ENABLED";
    case (int)RS2_OPTION_FRAMES_QUEUE_SIZE: return "RS2_OPTION_FRAMES_QUEUE_SIZE";
    case (int)RS2_OPTION_TOTAL_FRAME_DROPS: return "RS2_OPTION_TOTAL_FRAME_DROPS";
    case (int)RS2_OPTION_AUTO_EXPOSURE_MODE: return "RS2_OPTION_AUTO_EXPOSURE_MODE";
    case (int)RS2_OPTION_POWER_LINE_FREQUENCY: return "RS2_OPTION_POWER_LINE_FREQUENCY";
    case (int)RS2_OPTION_ASIC_TEMPERATURE: return "RS2_OPTION_ASIC_TEMPERATURE";
    case (int)RS2_OPTION_ERROR_POLLING_ENABLED: return "RS2_OPTION_ERROR_POLLING_ENABLED";
    case (int)RS2_OPTION_PROJECTOR_TEMPERATURE: return "RS2_OPTION_PROJECTOR_TEMPERATURE";
    case (int)RS2_OPTION_OUTPUT_TRIGGER_ENABLED: return "RS2_OPTION_OUTPUT_TRIGGER_ENABLED";
    case (int)RS2_OPTION_MOTION_MODULE_TEMPERATURE: return "RS2_OPTION_MOTION_MODULE_TEMPERATURE";
    case (int)RS2_OPTION_DEPTH_UNITS: return "RS2_OPTION_DEPTH_UNITS";
    case (int)RS2_OPTION_ENABLE_MOTION_CORRECTION: return "RS2_OPTION_ENABLE_MOTION_CORRECTION";
    case (int)RS2_OPTION_AUTO_EXPOSURE_PRIORITY: return "RS2_OPTION_AUTO_EXPOSURE_PRIORITY";
    case (int)RS2_OPTION_COLOR_SCHEME: return "RS2_OPTION_COLOR_SCHEME";
    case (int)RS2_OPTION_HISTOGRAM_EQUALIZATION_ENABLED: return "RS2_OPTION_HISTOGRAM_EQUALIZATION_ENABLED";
    case (int)RS2_OPTION_MIN_DISTANCE: return "RS2_OPTION_MIN_DISTANCE";
    case (int)RS2_OPTION_MAX_DISTANCE: return "RS2_OPTION_MAX_DISTANCE";
    case (int)RS2_OPTION_TEXTURE_SOURCE: return "RS2_OPTION_TEXTURE_SOURCE";
    case (int)RS2_OPTION_FILTER_MAGNITUDE: return "RS2_OPTION_FILTER_MAGNITUDE";
    case (int)RS2_OPTION_FILTER_SMOOTH_ALPHA: return "RS2_OPTION_FILTER_SMOOTH_ALPHA";
    case (int)RS2_OPTION_FILTER_SMOOTH_DELTA: return "RS2_OPTION_FILTER_SMOOTH_DELTA";
    case (int)RS2_OPTION_HOLES_FILL: return "RS2_OPTION_HOLES_FILL";
    case (int)RS2_OPTION_STEREO_BASELINE: return "RS2_OPTION_STEREO_BASELINE";
    case (int)RS2_OPTION_AUTO_EXPOSURE_CONVERGE_STEP: return "RS2_OPTION_AUTO_EXPOSURE_CONVERGE_STEP";
    case (int)RS2_OPTION_INTER_CAM_SYNC_MODE: return "RS2_OPTION_INTER_CAM_SYNC_MODE";
    case (int)RS2_OPTION_STREAM_FILTER: return "RS2_OPTION_STREAM_FILTER";
    case (int)RS2_OPTION_STREAM_FORMAT_FILTER: return "RS2_OPTION_STREAM_FORMAT_FILTER";
    case (int)RS2_OPTION_STREAM_INDEX_FILTER: return "RS2_OPTION_STREAM_INDEX_FILTER";
    case (int)RS2_OPTION_EMITTER_ON_OFF: return "RS2_OPTION_EMITTER_ON_OFF";
    case (int)RS2_OPTION_COUNT: return "RS2_OPTION_COUNT";
    default:
        return "NOT EXIST";
    }
}
/*#include <dshow.h>
#include <locale>
#include <vector>
using namespace std;

void _FreeMediaType(AM_MEDIA_TYPE& mt)
{
    if (mt.cbFormat != 0)
    {
        CoTaskMemFree((PVOID)mt.pbFormat);
        mt.cbFormat = 0;
        mt.pbFormat = NULL;
    }
    if (mt.pUnk != NULL)
    {
        // pUnk should not be used.
        mt.pUnk->Release();
        mt.pUnk = NULL;
    }
}


HRESULT CamCaps(IBaseFilter* pBaseFilter)
{
    HRESULT hr = 0;
    vector<IPin*> pins;
    IEnumPins* EnumPins;
    pBaseFilter->EnumPins(&EnumPins);
    pins.clear();
    for (;;)
    {
        IPin* pin;
        hr = EnumPins->Next(1, &pin, NULL);
        if (hr != S_OK) { break; }
        pins.push_back(pin);
        pin->Release();
    }
    EnumPins->Release();

    printf("Device pins number: %d\n", pins.size());

    PIN_INFO pInfo;
    for (int i = 0; i < pins.size(); i++)
    {
        pins[i]->QueryPinInfo(&pInfo);


        if (pInfo.dir == 0)
        {
            wprintf(L"Pin name: %s (§£§Ó§à§Õ)\n", pInfo.achName);
        }

        if (pInfo.dir == 1)
        {
            wprintf(L"Pin name: %s (§£§í§ç§à§Õ)\n", pInfo.achName);
        }

        IEnumMediaTypes* emt = NULL;
        pins[i]->EnumMediaTypes(&emt);

        AM_MEDIA_TYPE* pmt;

        vector<SIZE> modes;
        wprintf(L"Avialable resolutions.\n", pInfo.achName);
        for (;;)
        {
            hr = emt->Next(1, &pmt, NULL);
            if (hr != S_OK) { break; }

            if ((pmt->formattype == FORMAT_VideoInfo) &&
                //(pmt->subtype == MEDIASUBTYPE_RGB24) &&
                (pmt->cbFormat >= sizeof(VIDEOINFOHEADER)) &&
                (pmt->pbFormat != NULL))
            {
                VIDEOINFOHEADER* pVIH = (VIDEOINFOHEADER*)pmt->pbFormat;
                SIZE s;
                // Get frame size
                s.cy = pVIH->bmiHeader.biHeight;
                s.cx = pVIH->bmiHeader.biWidth;
                // §¢§Ú§ä§â§Ö§Û§ä
                unsigned int bitrate = pVIH->dwBitRate;
                modes.push_back(s);
                // Bits per pixel
                unsigned int bitcount = pVIH->bmiHeader.biBitCount;
                REFERENCE_TIME t = pVIH->AvgTimePerFrame; // blocks (100ns) per frame
                int FPS = floor(10000000.0 / static_cast<double>(t));


                printf("Size: x=%d\ty=%d\tFPS: %d\t bitrate: %ld\tbit/pixel:%ld\n", s.cx, s.cy, FPS, bitrate, bitcount);
            }
            _FreeMediaType(*pmt);
        }
        //----------------------------------------------------
        //
        //
        //
        //----------------------------------------------------
        modes.clear();
        emt->Release();
    }

    pins.clear();

    return S_OK;
}

/*
* Do something with the filter. In this sample we just test the pan/tilt properties.
*/
/*
void process_filter(IBaseFilter* pBaseFilter)
{
    CamCaps(pBaseFilter);
}


/*
* Enumerate all video devices
*
* See also:
*
* Using the System Device Enumerator:
*     http://msdn2.microsoft.com/en-us/library/ms787871.aspx
*/
/*
int enum_devices()
{
    HRESULT hr;
    printf("Enumeraring videoinput devices ...\n");

    // Create the System Device Enumerator.
    ICreateDevEnum* pSysDevEnum = NULL;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
        IID_ICreateDevEnum, (void**)&pSysDevEnum);
    if (FAILED(hr))
    {
        fprintf(stderr, "Error. Can't create enumerator.\n");
        return hr;
    }

    // Obtain a class enumerator for the video input device category.
    IEnumMoniker* pEnumCat = NULL;
    hr = pSysDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnumCat, 0);

    if (hr == S_OK)
    {
        // Enumerate the monikers.
        IMoniker* pMoniker = NULL;
        ULONG cFetched;
        while (pEnumCat->Next(1, &pMoniker, &cFetched) == S_OK)
        {
            IPropertyBag* pPropBag;
            hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag,
                (void**)&pPropBag);
            if (SUCCEEDED(hr))
            {
                // To retrieve the filter's friendly name, do the following:
                VARIANT varName;
                VariantInit(&varName);
                hr = pPropBag->Read(L"FriendlyName", &varName, 0);
                if (SUCCEEDED(hr))
                {
                    // Display the name in your UI somehow.
                    wprintf(L"------------------> %s <------------------\n", varName.bstrVal);
                }
                VariantClear(&varName);

                // To create an instance of the filter, do the following:
                IBaseFilter* pFilter;
                hr = pMoniker->BindToObject(NULL, NULL, IID_IBaseFilter,
                    (void**)&pFilter);

                process_filter(pFilter);

                //Remember to release pFilter later.
                pPropBag->Release();
            }
            pMoniker->Release();
        }
        pEnumCat->Release();
    }
    pSysDevEnum->Release();

    return 0;
}


int main(int argc, char* argv[])
{
    int result;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    result = enum_devices();

    CoUninitialize();
    getchar();

    return result;
}
*/