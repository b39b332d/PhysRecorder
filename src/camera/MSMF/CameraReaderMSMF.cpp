#include "CameraDriverMSMF.h"
#include <Mferror.h>
#include <chrono> 

#include <iostream>

namespace capture {

    CameraReaderMSMF::CameraReaderMSMF(CameraDevice* device) :
        device(device), m_nRefCount(0) {

    }

    HRESULT  CameraReaderMSMF::OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex,
        DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample* pSample)
    {//  thread safe!
        long long current_ts = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

        std::unique_lock l(read_sample_lock);
        HRESULT hr = 0;
        if (pSample != NULL) {
            CameraStreamMSMF* pstr;
            int pprof_n = 0;
            for (auto s : device->enabled_streams) {
                pstr = (CameraStreamMSMF*)s;
                if (pstr->stream_idx == dwStreamIndex)
                    break;
            }
            IMFMediaBuffer* pBuffer;
            BYTE* pBuf;
            DWORD LEN, CURRLEN;
            if (since == nullptr) since = new long long (current_ts - llTimestamp / 10);
            pSample->ConvertToContiguousBuffer(&pBuffer);
            pBuffer->Lock(&pBuf, &LEN, &CURRLEN);
            RawFrame* frame = pstr->get_current_profile()->createFrame(
                 llTimestamp/10+ *since, pBuf, CURRLEN, [pBuffer] {
                    pBuffer->Unlock();
                    pBuffer->Release();
                });
            pstr->write(frame);
            pstr->last_valid_ts = current_ts;
        }
        else if (FAILED(hrStatus)) {
            goto refresh_buffers;
        }
        hr = pReader->ReadSample(MF_SOURCE_READER_ANY_STREAM, 0, NULL, NULL, NULL, NULL);
        if (FAILED(hr)) {
            goto refresh_buffers;
        }
        return S_OK;
    refresh_buffers:
        l.unlock();
        device->stop(false);
        return S_OK;

    }
    HRESULT  CameraReaderMSMF::OnEvent(DWORD, IMFMediaEvent*)
    {
        return S_OK;
    }
    HRESULT  CameraReaderMSMF::OnFlush(DWORD)
    {
        return S_OK;
    }

    CameraReaderMSMF::~CameraReaderMSMF()
    {
        read_sample_lock.lock();
        device->onDeviceReadingFailed();
        read_sample_lock.unlock();
        if (since) {
            delete since;
        }
    }
};
