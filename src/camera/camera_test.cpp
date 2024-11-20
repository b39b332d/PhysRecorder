#define UNICODE
#include <iostream>
#include <new>
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <Wmcodecdsp.h>
#include <assert.h>
#include <Dbt.h>
#include <shlwapi.h>
#include <mfplay.h>
#include <mferror.h>

#include <iostream>
#include <vector>
#include <windows.h>
#include <opencv2/highgui.hpp>
#include <atlbase.h>
#include <atlcom.h>
const UINT WM_APP_PREVIEW_ERROR = WM_APP + 1;    // wparam = HRESULT


// The following code enables you to view the contents of a media type while 
// debugging.

#include <strsafe.h>


//#include "DeviceList.h"

/*
* A templated Function SafeRelease releasing pointers memories
* @param ppT the pointer to release
*/

template <class T> void SafeRelease(T** ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}


LPCWSTR GetGUIDNameConst(const GUID& guid);
HRESULT GetGUIDName(const GUID& guid, WCHAR** ppwsz);

HRESULT LogAttributeValueByIndex(IMFAttributes* pAttr, DWORD index);
HRESULT SpecialCaseAttributeValue(GUID guid, const PROPVARIANT& var);

void DBGMSG(PCWSTR format, ...);

HRESULT LogMediaType(IMFMediaType* pType)
{
    UINT32 count = 0;

    HRESULT hr = pType->GetCount(&count);
    if (FAILED(hr))
    {
        return hr;
    }

    if (count == 0)
    {
        DBGMSG(L"Empty media type.\n");
    }

    for (UINT32 i = 0; i < count; i++)
    {
        hr = LogAttributeValueByIndex(pType, i);
        if (FAILED(hr))
        {
            break;
        }
    }
    return hr;
}

HRESULT LogAttributeValueByIndex(IMFAttributes* pAttr, DWORD index)
{
    WCHAR* pGuidName = NULL;
    WCHAR* pGuidValName = NULL;

    GUID guid = { 0 };

    PROPVARIANT var;
    PropVariantInit(&var);

    HRESULT hr = pAttr->GetItemByIndex(index, &guid, &var);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = GetGUIDName(guid, &pGuidName);
    if (FAILED(hr))
    {
        goto done;
    }
    DBGMSG(L"\t%s\t", pGuidName);

    hr = SpecialCaseAttributeValue(guid, var);
    if (FAILED(hr))
    {
        goto done;
    }
    if (hr == S_FALSE)
    {
        switch (var.vt)
        {
        case VT_UI4:
            DBGMSG(L"%d", var.ulVal);
            break;

        case VT_UI8:
            DBGMSG(L"%I64d", var.uhVal);
            break;

        case VT_R8:
            DBGMSG(L"%f", var.dblVal);
            break;

        case VT_CLSID:
            hr = GetGUIDName(*var.puuid, &pGuidValName);
            if (SUCCEEDED(hr))
            {
                DBGMSG(pGuidValName);
            }
            break;

        case VT_LPWSTR:
            DBGMSG(var.pwszVal);
            break;

        case VT_VECTOR | VT_UI1:
            DBGMSG(L"<<byte array>>");
            break;

        case VT_UNKNOWN:
            DBGMSG(L"IUnknown");
            break;

        default:
            DBGMSG(L"Unexpected attribute type (vt = %d)", var.vt);
            break;
        }
    }

done:
    DBGMSG(L"\n");
    CoTaskMemFree(pGuidName);
    CoTaskMemFree(pGuidValName);
    PropVariantClear(&var);
    return hr;
}

HRESULT GetGUIDName(const GUID& guid, WCHAR** ppwsz)
{
    HRESULT hr = S_OK;
    WCHAR* pName = NULL;

    LPCWSTR pcwsz = GetGUIDNameConst(guid);
    if (pcwsz)
    {
        size_t cchLength = 0;

        hr = StringCchLength(pcwsz, STRSAFE_MAX_CCH, &cchLength);
        if (FAILED(hr))
        {
            goto done;
        }

        pName = (WCHAR*)CoTaskMemAlloc((cchLength + 1) * sizeof(WCHAR));

        if (pName == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }

        hr = StringCchCopy(pName, cchLength + 1, pcwsz);
        if (FAILED(hr))
        {
            goto done;
        }
    }
    else
    {
        hr = StringFromCLSID(guid, &pName);
    }

done:
    if (FAILED(hr))
    {
        *ppwsz = NULL;
        CoTaskMemFree(pName);
    }
    else
    {
        *ppwsz = pName;
    }
    return hr;
}

void LogUINT32AsUINT64(const PROPVARIANT& var)
{
    UINT32 uHigh = 0, uLow = 0;
    Unpack2UINT32AsUINT64(var.uhVal.QuadPart, &uHigh, &uLow);
    DBGMSG(L"%d x %d", uHigh, uLow);
}

float OffsetToFloat(const MFOffset& offset)
{
    return offset.value + (static_cast<float>(offset.fract) / 65536.0f);
}

HRESULT LogVideoArea(const PROPVARIANT& var)
{
    if (var.caub.cElems < sizeof(MFVideoArea))
    {
        return S_FALSE;
    }

    MFVideoArea* pArea = (MFVideoArea*)var.caub.pElems;

    DBGMSG(L"(%f,%f) (%d,%d)", OffsetToFloat(pArea->OffsetX), OffsetToFloat(pArea->OffsetY),
        pArea->Area.cx, pArea->Area.cy);
    return S_OK;
}

// Handle certain known special cases.
HRESULT SpecialCaseAttributeValue(GUID guid, const PROPVARIANT& var)
{
    if ((guid == MF_MT_FRAME_RATE) || (guid == MF_MT_FRAME_RATE_RANGE_MAX) ||
        (guid == MF_MT_FRAME_RATE_RANGE_MIN) || (guid == MF_MT_FRAME_SIZE) ||
        (guid == MF_MT_PIXEL_ASPECT_RATIO))
    {
        // Attributes that contain two packed 32-bit values.
        LogUINT32AsUINT64(var);
    }
    else if ((guid == MF_MT_GEOMETRIC_APERTURE) ||
        (guid == MF_MT_MINIMUM_DISPLAY_APERTURE) ||
        (guid == MF_MT_PAN_SCAN_APERTURE))
    {
        // Attributes that an MFVideoArea structure.
        return LogVideoArea(var);
    }
    else
    {
        return S_FALSE;
    }
    return S_OK;
}

void DBGMSG(PCWSTR format, ...)
{
    va_list args;
    va_start(args, format);

    WCHAR msg[MAX_PATH];

    if (SUCCEEDED(StringCbVPrintf(msg, sizeof(msg), format, args)))
    {
        std::wcout << msg;
    }
}

#ifndef IF_EQUAL_RETURN
#define IF_EQUAL_RETURN(param, val) if(val == param) return L#val
#endif

LPCWSTR GetGUIDNameConst(const GUID& guid)
{
    IF_EQUAL_RETURN(guid, MF_MT_MAJOR_TYPE);
    IF_EQUAL_RETURN(guid, MF_MT_MAJOR_TYPE);
    IF_EQUAL_RETURN(guid, MF_MT_SUBTYPE);
    IF_EQUAL_RETURN(guid, MF_MT_ALL_SAMPLES_INDEPENDENT);
    IF_EQUAL_RETURN(guid, MF_MT_FIXED_SIZE_SAMPLES);
    IF_EQUAL_RETURN(guid, MF_MT_COMPRESSED);
    IF_EQUAL_RETURN(guid, MF_MT_SAMPLE_SIZE);
    IF_EQUAL_RETURN(guid, MF_MT_WRAPPED_TYPE);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_NUM_CHANNELS);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_SAMPLES_PER_SECOND);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_FLOAT_SAMPLES_PER_SECOND);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_AVG_BYTES_PER_SECOND);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_BLOCK_ALIGNMENT);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_BITS_PER_SAMPLE);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_VALID_BITS_PER_SAMPLE);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_SAMPLES_PER_BLOCK);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_CHANNEL_MASK);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_FOLDDOWN_MATRIX);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_WMADRC_PEAKREF);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_WMADRC_PEAKTARGET);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_WMADRC_AVGREF);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_WMADRC_AVGTARGET);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_PREFER_WAVEFORMATEX);
    IF_EQUAL_RETURN(guid, MF_MT_AAC_PAYLOAD_TYPE);
    IF_EQUAL_RETURN(guid, MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION);
    IF_EQUAL_RETURN(guid, MF_MT_FRAME_SIZE);
    IF_EQUAL_RETURN(guid, MF_MT_FRAME_RATE);
    IF_EQUAL_RETURN(guid, MF_MT_FRAME_RATE_RANGE_MAX);
    IF_EQUAL_RETURN(guid, MF_MT_FRAME_RATE_RANGE_MIN);
    IF_EQUAL_RETURN(guid, MF_MT_PIXEL_ASPECT_RATIO);
    IF_EQUAL_RETURN(guid, MF_MT_DRM_FLAGS);
    IF_EQUAL_RETURN(guid, MF_MT_PAD_CONTROL_FLAGS);
    IF_EQUAL_RETURN(guid, MF_MT_SOURCE_CONTENT_HINT);
    IF_EQUAL_RETURN(guid, MF_MT_VIDEO_CHROMA_SITING);
    IF_EQUAL_RETURN(guid, MF_MT_INTERLACE_MODE);
    IF_EQUAL_RETURN(guid, MF_MT_TRANSFER_FUNCTION);
    IF_EQUAL_RETURN(guid, MF_MT_VIDEO_PRIMARIES);
    IF_EQUAL_RETURN(guid, MF_MT_CUSTOM_VIDEO_PRIMARIES);
    IF_EQUAL_RETURN(guid, MF_MT_YUV_MATRIX);
    IF_EQUAL_RETURN(guid, MF_MT_VIDEO_LIGHTING);
    IF_EQUAL_RETURN(guid, MF_MT_VIDEO_NOMINAL_RANGE);
    IF_EQUAL_RETURN(guid, MF_MT_GEOMETRIC_APERTURE);
    IF_EQUAL_RETURN(guid, MF_MT_MINIMUM_DISPLAY_APERTURE);
    IF_EQUAL_RETURN(guid, MF_MT_PAN_SCAN_APERTURE);
    IF_EQUAL_RETURN(guid, MF_MT_PAN_SCAN_ENABLED);
    IF_EQUAL_RETURN(guid, MF_MT_AVG_BITRATE);
    IF_EQUAL_RETURN(guid, MF_MT_AVG_BIT_ERROR_RATE);
    IF_EQUAL_RETURN(guid, MF_MT_MAX_KEYFRAME_SPACING);
    IF_EQUAL_RETURN(guid, MF_MT_DEFAULT_STRIDE);
    IF_EQUAL_RETURN(guid, MF_MT_PALETTE);
    IF_EQUAL_RETURN(guid, MF_MT_USER_DATA);
    IF_EQUAL_RETURN(guid, MF_MT_AM_FORMAT_TYPE);
    IF_EQUAL_RETURN(guid, MF_MT_MPEG_START_TIME_CODE);
    IF_EQUAL_RETURN(guid, MF_MT_MPEG2_PROFILE);
    IF_EQUAL_RETURN(guid, MF_MT_MPEG2_LEVEL);
    IF_EQUAL_RETURN(guid, MF_MT_MPEG2_FLAGS);
    IF_EQUAL_RETURN(guid, MF_MT_MPEG_SEQUENCE_HEADER);
    IF_EQUAL_RETURN(guid, MF_MT_DV_AAUX_SRC_PACK_0);
    IF_EQUAL_RETURN(guid, MF_MT_DV_AAUX_CTRL_PACK_0);
    IF_EQUAL_RETURN(guid, MF_MT_DV_AAUX_SRC_PACK_1);
    IF_EQUAL_RETURN(guid, MF_MT_DV_AAUX_CTRL_PACK_1);
    IF_EQUAL_RETURN(guid, MF_MT_DV_VAUX_SRC_PACK);
    IF_EQUAL_RETURN(guid, MF_MT_DV_VAUX_CTRL_PACK);
    IF_EQUAL_RETURN(guid, MF_MT_ARBITRARY_HEADER);
    IF_EQUAL_RETURN(guid, MF_MT_ARBITRARY_FORMAT);
    IF_EQUAL_RETURN(guid, MF_MT_IMAGE_LOSS_TOLERANT);
    IF_EQUAL_RETURN(guid, MF_MT_MPEG4_SAMPLE_DESCRIPTION);
    IF_EQUAL_RETURN(guid, MF_MT_MPEG4_CURRENT_SAMPLE_ENTRY);
    IF_EQUAL_RETURN(guid, MF_MT_ORIGINAL_4CC);
    IF_EQUAL_RETURN(guid, MF_MT_ORIGINAL_WAVE_FORMAT_TAG);

    // Media types

    IF_EQUAL_RETURN(guid, MFMediaType_Audio);
    IF_EQUAL_RETURN(guid, MFMediaType_Video);
    IF_EQUAL_RETURN(guid, MFMediaType_Protected);
    IF_EQUAL_RETURN(guid, MFMediaType_SAMI);
    IF_EQUAL_RETURN(guid, MFMediaType_Script);
    IF_EQUAL_RETURN(guid, MFMediaType_Image);
    IF_EQUAL_RETURN(guid, MFMediaType_HTML);
    IF_EQUAL_RETURN(guid, MFMediaType_Binary);
    IF_EQUAL_RETURN(guid, MFMediaType_FileTransfer);

    IF_EQUAL_RETURN(guid, MFVideoFormat_AI44); //     FCC('AI44')
    IF_EQUAL_RETURN(guid, MFVideoFormat_ARGB32); //   D3DFMT_A8R8G8B8 
    IF_EQUAL_RETURN(guid, MFVideoFormat_AYUV); //     FCC('AYUV')
    IF_EQUAL_RETURN(guid, MFVideoFormat_DV25); //     FCC('dv25')
    IF_EQUAL_RETURN(guid, MFVideoFormat_DV50); //     FCC('dv50')
    IF_EQUAL_RETURN(guid, MFVideoFormat_DVH1); //     FCC('dvh1')
    IF_EQUAL_RETURN(guid, MFVideoFormat_DVSD); //     FCC('dvsd')
    IF_EQUAL_RETURN(guid, MFVideoFormat_DVSL); //     FCC('dvsl')
    IF_EQUAL_RETURN(guid, MFVideoFormat_H264); //     FCC('H264')
    IF_EQUAL_RETURN(guid, MFVideoFormat_I420); //     FCC('I420')
    IF_EQUAL_RETURN(guid, MFVideoFormat_420O); //     FCC('I420')
    IF_EQUAL_RETURN(guid, MFVideoFormat_IYUV); //     FCC('IYUV')
    IF_EQUAL_RETURN(guid, MFVideoFormat_M4S2); //     FCC('M4S2')
    IF_EQUAL_RETURN(guid, MFVideoFormat_MJPG);
    IF_EQUAL_RETURN(guid, MFVideoFormat_MP43); //     FCC('MP43')
    IF_EQUAL_RETURN(guid, MFVideoFormat_MP4S); //     FCC('MP4S')
    IF_EQUAL_RETURN(guid, MFVideoFormat_MP4V); //     FCC('MP4V')
    IF_EQUAL_RETURN(guid, MFVideoFormat_MPG1); //     FCC('MPG1')
    IF_EQUAL_RETURN(guid, MFVideoFormat_MSS1); //     FCC('MSS1')
    IF_EQUAL_RETURN(guid, MFVideoFormat_MSS2); //     FCC('MSS2')
    IF_EQUAL_RETURN(guid, MFVideoFormat_NV11); //     FCC('NV11')
    IF_EQUAL_RETURN(guid, MFVideoFormat_NV12); //     FCC('NV12')
    IF_EQUAL_RETURN(guid, MFVideoFormat_P010); //     FCC('P010')
    IF_EQUAL_RETURN(guid, MFVideoFormat_P016); //     FCC('P016')
    IF_EQUAL_RETURN(guid, MFVideoFormat_P210); //     FCC('P210')
    IF_EQUAL_RETURN(guid, MFVideoFormat_P216); //     FCC('P216')
    IF_EQUAL_RETURN(guid, MFVideoFormat_RGB24); //    D3DFMT_R8G8B8 
    IF_EQUAL_RETURN(guid, MFVideoFormat_RGB32); //    D3DFMT_X8R8G8B8 
    IF_EQUAL_RETURN(guid, MFVideoFormat_RGB555); //   D3DFMT_X1R5G5B5 
    IF_EQUAL_RETURN(guid, MFVideoFormat_RGB565); //   D3DFMT_R5G6B5 
    IF_EQUAL_RETURN(guid, MFVideoFormat_RGB8);
    IF_EQUAL_RETURN(guid, MFVideoFormat_UYVY); //     FCC('UYVY')
    IF_EQUAL_RETURN(guid, MFVideoFormat_v210); //     FCC('v210')
    IF_EQUAL_RETURN(guid, MFVideoFormat_v410); //     FCC('v410')
    IF_EQUAL_RETURN(guid, MFVideoFormat_WMV1); //     FCC('WMV1')
    IF_EQUAL_RETURN(guid, MFVideoFormat_WMV2); //     FCC('WMV2')
    IF_EQUAL_RETURN(guid, MFVideoFormat_WMV3); //     FCC('WMV3')
    IF_EQUAL_RETURN(guid, MFVideoFormat_WVC1); //     FCC('WVC1')
    IF_EQUAL_RETURN(guid, MFVideoFormat_Y210); //     FCC('Y210')
    IF_EQUAL_RETURN(guid, MFVideoFormat_Y216); //     FCC('Y216')
    IF_EQUAL_RETURN(guid, MFVideoFormat_Y410); //     FCC('Y410')
    IF_EQUAL_RETURN(guid, MFVideoFormat_Y416); //     FCC('Y416')
    IF_EQUAL_RETURN(guid, MFVideoFormat_Y41P);
    IF_EQUAL_RETURN(guid, MFVideoFormat_Y41T);
    IF_EQUAL_RETURN(guid, MFVideoFormat_YUY2); //     FCC('YUY2')
    IF_EQUAL_RETURN(guid, MFVideoFormat_YV12); //     FCC('YV12')
    IF_EQUAL_RETURN(guid, MFVideoFormat_YVYU);


    IF_EQUAL_RETURN(guid, MFVideoFormat_L8);
    IF_EQUAL_RETURN(guid, MFVideoFormat_L16);
    IF_EQUAL_RETURN(guid, MFVideoFormat_D16);

    IF_EQUAL_RETURN(guid, MFAudioFormat_PCM); //              WAVE_FORMAT_PCM 
    IF_EQUAL_RETURN(guid, MFAudioFormat_Float); //            WAVE_FORMAT_IEEE_FLOAT 
    IF_EQUAL_RETURN(guid, MFAudioFormat_DTS); //              WAVE_FORMAT_DTS 
    IF_EQUAL_RETURN(guid, MFAudioFormat_Dolby_AC3_SPDIF); //  WAVE_FORMAT_DOLBY_AC3_SPDIF 
    IF_EQUAL_RETURN(guid, MFAudioFormat_DRM); //              WAVE_FORMAT_DRM 
    IF_EQUAL_RETURN(guid, MFAudioFormat_WMAudioV8); //        WAVE_FORMAT_WMAUDIO2 
    IF_EQUAL_RETURN(guid, MFAudioFormat_WMAudioV9); //        WAVE_FORMAT_WMAUDIO3 
    IF_EQUAL_RETURN(guid, MFAudioFormat_WMAudio_Lossless); // WAVE_FORMAT_WMAUDIO_LOSSLESS 
    IF_EQUAL_RETURN(guid, MFAudioFormat_WMASPDIF); //         WAVE_FORMAT_WMASPDIF 
    IF_EQUAL_RETURN(guid, MFAudioFormat_MSP1); //             WAVE_FORMAT_WMAVOICE9 
    IF_EQUAL_RETURN(guid, MFAudioFormat_MP3); //              WAVE_FORMAT_MPEGLAYER3 
    IF_EQUAL_RETURN(guid, MFAudioFormat_MPEG); //             WAVE_FORMAT_MPEG 
    IF_EQUAL_RETURN(guid, MFAudioFormat_AAC); //              WAVE_FORMAT_MPEG_HEAAC 
    IF_EQUAL_RETURN(guid, MFAudioFormat_ADTS); //             WAVE_FORMAT_MPEG_ADTS_AAC 

    return NULL;
}

HRESULT EnumerateCaptureFormats(IMFMediaSource* pSource)
{
    IMFPresentationDescriptor* pPD = NULL;
    IMFStreamDescriptor* pSD = NULL;
    IMFMediaTypeHandler* pHandler = NULL;
    IMFMediaType* pType = NULL;

    BOOL fSelected;
    DWORD cTypes = 0;
    HRESULT hr = pSource->CreatePresentationDescriptor(&pPD);
    if (FAILED(hr))
    {
        goto ECFdone;
    }

    hr = pPD->GetStreamDescriptorByIndex(0, &fSelected, &pSD);
    if (FAILED(hr))
    {
        goto ECFdone;
    }

    hr = pSD->GetMediaTypeHandler(&pHandler);
    if (FAILED(hr))
    {
        goto ECFdone;
    }

    hr = pHandler->GetMediaTypeCount(&cTypes);
    if (FAILED(hr))
    {
        goto ECFdone;
    }

    for (DWORD i = 0; i < cTypes; i++)
    {
        hr = pHandler->GetMediaTypeByIndex(i, &pType);
        if (FAILED(hr))
        {
            goto ECFdone;
        }

        LogMediaType(pType);
        std::cout << std::endl;

        SafeRelease(&pType);
    }

ECFdone:
    SafeRelease(&pPD);
    SafeRelease(&pSD);
    SafeRelease(&pHandler);
    SafeRelease(&pType);
    return hr;
}
class DeviceList
{
    UINT32      m_cDevices; // contains the number of devices
    IMFActivate** m_ppDevices; // contains properties about each device

public:
    DeviceList() : m_ppDevices(NULL), m_cDevices(0)
    {
        MFStartup(MF_VERSION);
    }
    ~DeviceList()
    {
        Clear();
    }

    UINT32  Count() const { return m_cDevices; }

    void    Clear();
    HRESULT EnumerateDevices();
    HRESULT EnumerateDeviceFormats();
    HRESULT GetDevice(UINT32 index, IMFActivate** ppActivate);
    HRESULT GetDeviceName(UINT32 index, WCHAR** ppszName);
};




/*
* A function which copy attribute form source to a destination
* @ param pSrc is an Interface to store key/value pairs of an Object
* @ param pDest is an Interface to store key/value pairs of an Object
* @ param GUID is an unique identifier
* @ return HRESULT return errors warning condition on windows
*/

HRESULT CopyAttribute(IMFAttributes* pSrc, IMFAttributes* pDest, const GUID& key);



/*
* A Method form DeviceList which clear the list of Devices
*/

void DeviceList::Clear()
{
    for (UINT32 i = 0; i < m_cDevices; i++)
    {
        SafeRelease(&m_ppDevices[i]);
    }
    CoTaskMemFree(m_ppDevices);
    m_ppDevices = NULL;

    m_cDevices = 0;
}


/*
* A function which enumerate the list of Devices.
* @ return HRESULT return errors warning condition on windows
*/
HRESULT DeviceList::EnumerateDevices()
{
    HRESULT hr = S_OK;
    IMFAttributes* pAttributes = NULL;

    this->Clear();

    // Initialize an attribute store. We will use this to
    // specify the enumeration parameters.
    std::cout << "Enumerate devices" << std::endl;
    hr = MFCreateAttributes(&pAttributes, 1);

    // Ask for source type = video capture devices
    if (SUCCEEDED(hr))
    {
        std::cout << "Enumerate devices" << std::endl;
        hr = pAttributes->SetGUID(
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
        );
    }
    // Enumerate devices.
    if (SUCCEEDED(hr))
    {
        std::cout << "Enumerate devices:" << m_cDevices << std::endl;
        hr = MFEnumDeviceSources(pAttributes, &m_ppDevices, &m_cDevices);
    }

    SafeRelease(&pAttributes);

    return hr;
}

HRESULT ConfigureDecoder(IMFSourceReader* pReader, DWORD dwStreamIndex, IMFMediaType* pNativeType)
{
    IMFMediaType* pType;

    GUID majorType, subtype;
    UINT32 size_width, size_height, fs_n,fs_d;

    // Find the major type.
    HRESULT  hr = pNativeType->GetGUID(MF_MT_MAJOR_TYPE, &majorType);
    MFGetAttributeSize(pNativeType, MF_MT_FRAME_SIZE, &size_width, &size_height);
    MFGetAttributeRatio(pNativeType, MF_MT_FRAME_RATE, &fs_n, &fs_d);
    if (FAILED(hr))
    {
        goto done;
    }

    // Define the output type.
    hr = MFCreateMediaType(&pType);
    if (FAILED(hr))
    {
        goto done;
    }
    //hr = pNativeType->CopyAllItems(pType);
    if (FAILED(hr))
    {
        goto done;
    }

    // Select a subtype.
    if (majorType == MFMediaType_Video)
    {
        subtype = MFVideoFormat_RGB32;
    }
    else if (majorType == MFMediaType_Audio)
    {
        subtype = MFAudioFormat_PCM;
    }
    else
    {
        // Unrecognized type. Skip.
        goto done;
    }

    hr = pType->SetGUID(MF_MT_MAJOR_TYPE, majorType);
    hr = pType->SetGUID(MF_MT_SUBTYPE, subtype);
    //MFSetAttributeSize(pType, MF_MT_FRAME_SIZE, size_width, size_height);
    //MFSetAttributeRatio(pType, MF_MT_FRAME_RATE, fs_n, fs_d);
    if (FAILED(hr))
    {
        goto done;
    }

    // Set the uncompressed format.
    hr = pReader->SetCurrentMediaType(dwStreamIndex, NULL, pType);
    std::cout << hr << std::endl;
    std::cout << MF_E_TOPO_CODEC_NOT_FOUND << std::endl;
    std::cout << MF_E_INVALIDMEDIATYPE << std::endl;
    std::cout << MF_E_INVALIDREQUEST << std::endl;
    std::cout << MF_E_INVALIDSTREAMNUMBER << std::endl;

    {
        IMFMediaType* apType = NULL;
        hr = pReader->GetCurrentMediaType(dwStreamIndex, &apType);
        LogMediaType(apType);
        SafeRelease(&apType);
    }

    if (FAILED(hr))
    {
        goto done;
    }

done:
    SafeRelease(&pType);
    return hr;
}
IMFSample* ReadFromReader(IMFSourceReader* pReader, DWORD dwStreamIndex, IMFMediaType* pType) {
    HRESULT hr = S_OK, idx_hr = S_OK;
    hr = ConfigureDecoder(pReader, dwStreamIndex, pType);
    if (FAILED(hr)) {
        std::cout << "SetCurrentMediaType error" << std::endl;
        return NULL;
    }
    DWORD streamIndex, flags;
    LONGLONG llTimeStamp;
    IMFSample* pSample= NULL;
reget:
    hr = pReader->ReadSample(MF_SOURCE_READER_ANY_STREAM, 0, &streamIndex, &flags, &llTimeStamp, &pSample);


    if (FAILED(hr))
    {
        return NULL;
    }

    wprintf(L"Stream %d (%I64d)\n", streamIndex, llTimeStamp);
    if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
    {
        wprintf(L"\tEnd of stream\n");
    }
    if (flags & MF_SOURCE_READERF_NEWSTREAM)
    {
        wprintf(L"\tNew stream\n");
    }
    if (flags & MF_SOURCE_READERF_NATIVEMEDIATYPECHANGED)
    {
        wprintf(L"\tNative type changed\n");
    }
    if (flags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED)
    {
        wprintf(L"\tCurrent type changed\n");
    }
    if (flags & MF_SOURCE_READERF_STREAMTICK)
    {
        wprintf(L"\tStream tick\n");
    }

    if (flags & MF_SOURCE_READERF_NATIVEMEDIATYPECHANGED)
    {
        // The format changed. Reconfigure the decoder.
        hr = ConfigureDecoder(pReader, streamIndex, pType);
        if (FAILED(hr))
        {
            return NULL;
        }
        goto reget;
    }

    if (FAILED(hr)) {
        std::cout << "ReadSample error" << std::endl;
    }
    else if (pSample == NULL) {
        goto reget;
    }
    else {
        return pSample;
    }
}
HRESULT EnumerateTypesForStream(IMFSourceReader* pReader, DWORD dwStreamIndex)
{
    HRESULT hr = S_OK, idx_hr = S_OK;
    DWORD dwMediaTypeIndex = 0;

    while (SUCCEEDED(idx_hr))
    {
        std::cout << "mediatype " << dwMediaTypeIndex << std::endl;
        IMFMediaType* pType = NULL;
        idx_hr = pReader->GetNativeMediaType(dwStreamIndex, dwMediaTypeIndex, &pType);
        if (idx_hr == MF_E_NO_MORE_TYPES)
        {
            idx_hr = S_OK;
            break;
        }
        else if (SUCCEEDED(idx_hr))
        {
           // LogMediaType(pType);
            if (dwMediaTypeIndex ==0 ) {
                // Examine the media type. (Not shown.)
                //auto pSample = ReadFromReader(pReader, dwStreamIndex, pType);
                //if (pSample != NULL) {
                //    IMFMediaBuffer* pBuffer;
                //    BYTE* pBuf;
                //    DWORD LEN, CURRLEN;
                //    pSample->ConvertToContiguousBuffer(&pBuffer);
                //    pBuffer->Lock(&pBuf, &LEN, &CURRLEN);
                //    UINT32 size_width, size_height, fs_n, fs_d;
                //    MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &size_width, &size_height);
                //    cv::Mat out(cv::Size(size_width, size_height), CV_8UC3, pBuf);
                //    cv::imshow("", out);
                //    cv::waitKey(1);
                //    std::cout << LEN << CURRLEN << std::endl;
                //    pBuffer->Unlock();
                //    pBuffer->Release();
                //    SafeRelease(&pSample);
                //}
            }
            pType->Release();
        }
        ++dwMediaTypeIndex;
    }
    return idx_hr;
}
HRESULT EnumerateMediaTypes(IMFSourceReader* pReader)
{
    HRESULT hr = S_OK;
    DWORD dwStreamIndex = 0;

    while (SUCCEEDED(hr))
    {
        std::cout << "Stream " << dwStreamIndex << std::endl;
        
        hr = EnumerateTypesForStream(pReader, dwStreamIndex);
        if (hr == MF_E_INVALIDSTREAMNUMBER)
        {
            hr = S_OK;
            break;
        }
        ++dwStreamIndex;
    }
    return hr;
}


HRESULT DeviceList::EnumerateDeviceFormats() {
    IMFMediaSource* pSource = NULL;
    HRESULT hr = S_OK;
    for (int i = 0; i < this->Count();i++) {
        IMFSourceReader* pReader = NULL;
        std::cout << "------------------ " << std::endl;
        std::cout << "Index " << i << std::endl;
        hr = m_ppDevices[i]->ActivateObject(IID_PPV_ARGS(&pSource));
        WCHAR* pszName;
        hr = m_ppDevices[i]->GetAllocatedString(
            MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
            &pszName,
            NULL
        );
        std::wstring ws(pszName);
        std::string str(ws.begin(), ws.end());
        std::cout << str << std::endl;
        if (FAILED(hr))
        {
            goto done;
        }

        IMFAttributes* pAttributes = nullptr;
        HRESULT hr = MFCreateAttributes(&pAttributes, 1);  // Create attributes with a capacity of 1
        if (FAILED(hr)) {
            goto done;
        }

        // Enable video processing (e.g., color conversion, scaling)
        hr = pAttributes->SetUINT32(MF_READWRITE_DISABLE_CONVERTERS , TRUE);
        if (FAILED(hr)) {
            pAttributes->Release();
            goto done;
        }

        hr = MFCreateSourceReaderFromMediaSource(pSource, pAttributes, &pReader);
        pAttributes->Release();
        if (FAILED(hr))
        {
            goto done;
        }
        // EnumerateMediaTypes(pReader);
        // 2 31
        
            IMFMediaType* pType = NULL;
            IMFMediaType* pType1 = NULL;
            IMFSourceReaderEx* pReaderEX=NULL;
            pReader->QueryInterface(&pReaderEX);
            hr = pReaderEX->GetNativeMediaType(0, 0, &pType);
            LogMediaType(pType);
            //hr = pReader->SetCurrentMediaType(1, NULL, pType1);
            //hr = pReader->SetCurrentMediaType(0, NULL, pType);
            //hr = pReader->SetStreamSelection(0, TRUE);
            //hr = pReader->SetStreamSelection(1, TRUE);
            CameraControlProperty clist[] = { CameraControl_Pan,
                              CameraControl_Tilt,
                              CameraControl_Roll,
                              CameraControl_Zoom,
                              CameraControl_Exposure,
                              CameraControl_Iris,
                              CameraControl_Focus };
            const char* clist_name[] = { 
                "CameraControl_Pan",
                  "CameraControl_Tilt",
                  "CameraControl_Roll",
                  "CameraControl_Zoom",
                  "CameraControl_Exposure",
                  "CameraControl_Iris",
                  "CameraControl_Focus" };
            tagVideoProcAmpProperty  vlist[] = { VideoProcAmp_Brightness,
                              VideoProcAmp_Contrast,
                              VideoProcAmp_Hue,
                              VideoProcAmp_Saturation,
                              VideoProcAmp_Sharpness,
                              VideoProcAmp_Gamma,
                              VideoProcAmp_ColorEnable,
                              VideoProcAmp_WhiteBalance,
                              VideoProcAmp_BacklightCompensation,
                              VideoProcAmp_Gain };
            const char* vlist_name[] = { "VideoProcAmp_Brightness",
                              "VideoProcAmp_Contrast",
                              "VideoProcAmp_Hue",
                              "VideoProcAmp_Saturation",
                              "VideoProcAmp_Sharpness",
                              "VideoProcAmp_Gamma",
                              "VideoProcAmp_ColorEnable",
                              "VideoProcAmp_WhiteBalance",
                              "VideoProcAmp_BacklightCompensation",
                              "VideoProcAmp_Gain" };


            //CComQIPtr<IAMVideoProcAmp> spVideo(pSource);
            //if (spVideo)
            //    hr = spVideo->Set(VideoProcAmp_WhiteBalance, 0, VideoProcAmp_Flags_Auto);

            CComQIPtr<IAMCameraControl> spCameraControl;
            pSource->QueryInterface(IID_IAMCameraControl, (void**)&spCameraControl);

            if (spCameraControl) {
                for (int i = 0; i < 7; i++) {
                    long min, max, step, def, control;
                    std::cout << clist_name[i] << "\t";
                    hr = spCameraControl->GetRange(clist[i], &min, &max, &step, &def, &control);
                    if (SUCCEEDED(hr)) {
                        std::cout << "min" << min << "\tmax" << max << "\tstep" << step << "\tdef" << def;
                        if (control == CameraControl_Flags_Auto) std::cout << "\tauto" << std::endl;
                        else if (control == CameraControl_Flags_Manual) std::cout << "\tmanual" << std::endl;
                        else std::cout << "\tboth" << std::endl;
                        continue;
                    }
                    std::cout << "not support!" << std::endl;
                }
            }
            CComQIPtr<IAMVideoProcAmp> spVideoControl;
            pSource->QueryInterface(IID_IAMVideoProcAmp, (void**)&spVideoControl);
            if (spCameraControl) {
                for (int i = 0; i < 10; i++) {
                    long min, max, step, def, control;
                    std::cout << vlist_name[i] << "\t";
                    hr = spVideoControl->GetRange(vlist[i], &min, &max, &step, &def, &control);
                    if (SUCCEEDED(hr)) {
                        std::cout << "min" << min << "\tmax" << max << "\tstep" << step << "\tdef" << def;
                        if (control == VideoProcAmp_Flags_Auto) std::cout << "\tauto" << std::endl;
                        else if (control == VideoProcAmp_Flags_Manual) std::cout << "\tmanual" << std::endl;
                        else std::cout << "\tboth" << std::endl;
                        continue;
                    }
                    std::cout << "not support!" << std::endl;
                }
                continue;


            DWORD streamIndex, flags;
            LONGLONG llTimeStamp;
            IMFSample* pSample;
        reget:
            hr = pReader->ReadSample(MF_SOURCE_READER_ANY_STREAM, 0, &streamIndex, &flags, &llTimeStamp, &pSample);

            std::cout << hr << std::endl;
            std::cout << MF_E_INVALIDREQUEST << std::endl;
            std::cout << MF_E_INVALIDSTREAMNUMBER << std::endl;
            std::cout << MF_E_NOTACCEPTING << std::endl;
            std::cout << E_INVALIDARG << std::endl; 
            if (pSample == NULL)
                goto reget;

            IMFMediaBuffer* pBuffer;
            BYTE* pBuf;
            DWORD LEN, CURRLEN;
            pSample->ConvertToContiguousBuffer(&pBuffer);
            pBuffer->Lock(&pBuf, &LEN, &CURRLEN);
            UINT32 size_width, size_height, fs_n, fs_d;
            MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &size_width, &size_height);
            cv::Mat out(cv::Size(size_width, size_height), CV_8UC3, pBuf);
            //cv::imshow("", out);
            //cv::waitKey(1);
            std::cout << LEN << CURRLEN << std::endl;
            pBuffer->Unlock();
            pBuffer->Release();
            SafeRelease(&pSample);
            pSample = NULL;




            goto reget;

        }
        


        pReader->Release();
        //EnumerateCaptureFormats(pSource);
    }

done:
    return hr;
}
/*
* A function which copy attribute form source to a destination
* @ param index the index in an array
* @ param ppActivate is an Interface to store key/value pairs of an Object
* @ return HRESULT return errors warning condition on windows
*/


HRESULT DeviceList::GetDevice(UINT32 index, IMFActivate** ppActivate)
{
    if (index >= Count())
    {
        return E_INVALIDARG;
    }

    *ppActivate = m_ppDevices[index];
    (*ppActivate)->AddRef();

    return S_OK;
}


/*
* A function which get the name of the devices
* @ param index the index in an array
* @ param ppszName Name of the device
*/


HRESULT DeviceList::GetDeviceName(UINT32 index, WCHAR** ppszName)
{
    std::cout << "Get Device name" << std::endl;
    if (index >= Count())
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    hr = m_ppDevices[index]->GetAllocatedString(
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
        ppszName,
        NULL
    );

    return hr;

}


std::shared_ptr<std::vector<WCHAR*>> GetDeviceList()
{
    HRESULT hr = S_OK;

    DeviceList g_devices;


    g_devices.Clear();

    hr = g_devices.EnumerateDevices();


    std::shared_ptr < std::vector<WCHAR*>> device_id(new std::vector<WCHAR*>, [](std::vector<WCHAR*>* p) {
        for (auto i : *p) {
            CoTaskMemFree(i);
        }
        delete p;
        });
    if (FAILED(hr)) { goto done; }

    for (UINT32 iDevice = 0; iDevice < g_devices.Count(); iDevice++)
    {
        WCHAR* szFriendlyName = NULL;
        IMFActivate* pActivate;
        hr = g_devices.GetDeviceName(iDevice, &szFriendlyName);
        device_id->push_back(szFriendlyName);
        if (FAILED(hr)) { goto done; }
    }
done:
    return device_id;
}


int main() {
    HRESULT hr = S_OK;

    DeviceList g_devices;
    hr = g_devices.EnumerateDevices();
    hr = g_devices.EnumerateDeviceFormats();

    g_devices.Clear();

	return 0;
}