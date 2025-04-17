#ifndef _CAMERADRIVER_MSMF_H_
#define _CAMERADRIVER_MSMF_H_


#include <string>
#include <CameraDriver.h>
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#include <shlwapi.h>
#define THROW_HR(HR,STR) if(FAILED(HR)) throw std::exception(STR)

namespace capture {
    class CameraStreamMSMF;
    class CameraDevice;
    class CameraProfileMSMF :public CameraProfile {
    public:
        IMFMediaType* pType;
        DWORD dwStreamIndex;

        CameraProfileMSMF(CameraStreamMSMF* stream, IMFMediaType* pType);
        ~CameraProfileMSMF();
    };
    struct profile_buffer_t;
    class CameraReaderMSMF : public IMFSourceReaderCallback {

        std::mutex buffers_lock;
        bool buffer_refresh = false;
        std::mutex read_sample_lock;
        bool valid = true;
        long long* since=nullptr; // microseconds 
    public:
        IMFSourceReader* pReader;
        CameraReaderMSMF(CameraDevice* device);
        STDMETHODIMP QueryInterface(REFIID iid, void** ppv)
        {
            static const QITAB qit[] =
            {
                QITABENT(CameraReaderMSMF, IMFSourceReaderCallback),
                { 0 },
            };
            return QISearch(this, qit, iid, ppv);
        }
        STDMETHODIMP OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex,
            DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample* pSample);
        STDMETHODIMP OnEvent(DWORD, IMFMediaEvent*);
        STDMETHODIMP OnFlush(DWORD);
        STDMETHODIMP_(ULONG) AddRef() {
            return InterlockedIncrement(&m_nRefCount);
        };
        STDMETHODIMP_(ULONG) Release() {
            ULONG uCount = InterlockedDecrement(&m_nRefCount);
            if (uCount == 0)
            {
                delete this;
            }
            return uCount;
        };
        long                m_nRefCount;
    private:
        ~CameraReaderMSMF();
        CameraDevice* device;
    };

    class CameraDeviceMSMF;
    class CameraStreamMSMF :public CameraStream {
    public:
        DWORD stream_idx;
        CameraStreamMSMF(const std::string& stream_name,CameraDeviceMSMF* pdevice, IMFSourceReader* pReader, DWORD stream_idx, IMFMediaType* default_native_profile);

    };
    class CameraDeviceMSMF :public CameraDevice {
        IMFActivate* m_pDevices; // contains properties about each device
        IMFMediaSource* pSource = nullptr;
        IMFSourceReader* pReader;
        CameraReaderMSMF* cam_reader;
    public:
        CameraDeviceMSMF(IMFActivate* m_pDevices);


        bool native_init();
        bool native_start();
        void native_stop();
        void native_release();


        template<class controller>
        option_status get_signle_option_spCamera(controller& spControl, long opt_id);
        template<class controller>
        void set_signle_option_spCamera(controller& spControl, const long& opt_id, option_range& range, const option_status& option);
        //IAMCameraControl* spCameraControl=nullptr;
        //IAMVideoProcAmp* spVideoControl = nullptr;
        bool use_gain;
        void get_all_option_range_native(IMFMediaSource* pSource);
        option_status get_option_native(DEVICE_OPTION option);
        void set_option_native(DEVICE_OPTION option, const option_status& value);
    };

    std::vector<CameraDevice*> EnumerateCamera_MSMF();
};
#endif