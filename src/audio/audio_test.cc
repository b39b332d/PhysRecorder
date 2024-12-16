#include <iostream>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <propvarutil.h>
#include <vector>
#include <string>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "propsys.lib")

HRESULT EnumerateAudioDevices(std::vector<IMFActivate*>& devices) {
    IMFAttributes* pAttributes = NULL;
    IMFActivate** ppDevices = NULL;
    UINT32 count;

    HRESULT hr = MFCreateAttributes(&pAttributes, 1);
    if (SUCCEEDED(hr)) {
        hr = pAttributes->SetGUID(
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_GUID
        );
    }

    if (SUCCEEDED(hr)) {
        hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
    }

    if (SUCCEEDED(hr)) {
        for (UINT32 i = 0; i < count; i++) {
            devices.push_back(ppDevices[i]);
        }
    }

    if (pAttributes) pAttributes->Release();
    if (ppDevices) CoTaskMemFree(ppDevices);

    return hr;
}

HRESULT GetDeviceFriendlyName(IMFActivate* pActivate, std::wstring& friendlyName) {
    WCHAR* szFriendlyName = NULL;
    UINT32 cchName;

    HRESULT hr = pActivate->GetAllocatedString(
        MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
        &szFriendlyName,
        &cchName
    );

    if (SUCCEEDED(hr)) {
        friendlyName = szFriendlyName;
        CoTaskMemFree(szFriendlyName);
    }

    return hr;
}

HRESULT EnumerateProfiles(IMFMediaSource* pSource, std::vector<IMFMediaType*>& profiles) {
    IMFPresentationDescriptor* pPD = NULL;
    IMFStreamDescriptor* pSD = NULL;
    IMFMediaTypeHandler* pHandler = NULL;

    HRESULT hr = pSource->CreatePresentationDescriptor(&pPD);
    if (SUCCEEDED(hr)) {
        BOOL fSelected;
        hr = pPD->GetStreamDescriptorByIndex(0, &fSelected, &pSD);
    }

    if (SUCCEEDED(hr)) {
        hr = pSD->GetMediaTypeHandler(&pHandler);
    }

    if (SUCCEEDED(hr)) {
        DWORD count;
        hr = pHandler->GetMediaTypeCount(&count);
        for (DWORD i = 0; i < count && SUCCEEDED(hr); i++) {
            IMFMediaType* pMediaType = NULL;
            hr = pHandler->GetMediaTypeByIndex(i, &pMediaType);
            if (SUCCEEDED(hr)) {
                profiles.push_back(pMediaType);
            }
        }
    }

    if (pPD) pPD->Release();
    if (pSD) pSD->Release();
    if (pHandler) pHandler->Release();

    return hr;
}

HRESULT GetAudioProfileInfo(IMFMediaType* pMediaType, std::wstring& info) {
    UINT32 channels, samplesPerSec, bitsPerSample;
    HRESULT hr = pMediaType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channels);
    if (SUCCEEDED(hr)) {
        hr = pMediaType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &samplesPerSec);
    }
    if (SUCCEEDED(hr)) {
        hr = pMediaType->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &bitsPerSample);
    }

    if (SUCCEEDED(hr)) {
        info = L"Channels: " + std::to_wstring(channels) +
            L", Sample Rate: " + std::to_wstring(samplesPerSec) +
            L", Bits Per Sample: " + std::to_wstring(bitsPerSample);
    }

    return hr;
}

HRESULT RecordAudio(IMFMediaSource* pSource, IMFMediaType* pMediaType, const std::wstring& outputFileName) {
    IMFSinkWriter* pSinkWriter = NULL;
    DWORD streamIndex;

    // Create a full path for the output file
    wchar_t fullPath[MAX_PATH];
    if (GetFullPathNameW(outputFileName.c_str(), MAX_PATH, fullPath, NULL) == 0) {
        std::wcerr << L"Failed to get full path for output file" << std::endl;
        return E_FAIL;
    }

    // Create an attribute store
    IMFAttributes* pAttributes = NULL;
    HRESULT hr = MFCreateAttributes(&pAttributes, 1);
    if (FAILED(hr)) {
        std::wcerr << L"Failed to create attributes: 0x" << std::hex << hr << std::endl;
        return hr;
    }

    // Set the readwrite attribute
    hr = pAttributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
    if (FAILED(hr)) {
        std::wcerr << L"Failed to set readwrite attribute: 0x" << std::hex << hr << std::endl;
        pAttributes->Release();
        return hr;
    }

    // Create the sink writer
    hr = MFCreateSinkWriterFromURL(fullPath, NULL, pAttributes, &pSinkWriter);
    if (FAILED(hr)) {
        std::wcerr << L"MFCreateSinkWriterFromURL failed: 0x" << std::hex << hr << std::endl;
        pAttributes->Release();
        return hr;
    }

    if (SUCCEEDED(hr)) {
        hr = pSinkWriter->AddStream(pMediaType, &streamIndex);
    }

    if (SUCCEEDED(hr)) {
        hr = pSinkWriter->SetInputMediaType(streamIndex, pMediaType, NULL);
    }

    if (SUCCEEDED(hr)) {
        hr = pSinkWriter->BeginWriting();
    }

    if (SUCCEEDED(hr)) {
        IMFMediaBuffer* pBuffer = NULL;
        IMFSample* pSample = NULL;
        DWORD cbBuffer = 0;
        BYTE* pData = NULL;

        // Record for 5 seconds
        for (int i = 0; i < 5 && SUCCEEDED(hr); i++) {
            hr = MFCreateMemoryBuffer(44100 * 4, &pBuffer); // Assuming 44.1kHz, 16-bit stereo
            if (SUCCEEDED(hr)) {
                hr = pBuffer->Lock(&pData, NULL, &cbBuffer);
            }
            if (SUCCEEDED(hr)) {
                // In a real application, you would read audio data from the source here
                // For this example, we're just writing silence
                memset(pData, 0, cbBuffer);
                hr = pBuffer->Unlock();
                pBuffer->SetCurrentLength(cbBuffer);
            }
            if (SUCCEEDED(hr)) {
                hr = MFCreateSample(&pSample);
            }
            if (SUCCEEDED(hr)) {
                hr = pSample->AddBuffer(pBuffer);
            }
            if (SUCCEEDED(hr)) {
                LONGLONG hnsSampleTime = i * 10000000LL; // 100-nanosecond units
                hr = pSample->SetSampleTime(hnsSampleTime);
            }
            if (SUCCEEDED(hr)) {
                hr = pSinkWriter->WriteSample(streamIndex, pSample);
            }

            if (pSample) pSample->Release();
            if (pBuffer) pBuffer->Release();
        }
    }

    if (SUCCEEDED(hr)) {
        hr = pSinkWriter->Finalize();
    }

    if (pSinkWriter) pSinkWriter->Release();

    return hr;
}

int main() {
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        std::wcerr << L"Failed to initialize COM library" << std::endl;
        return 1;
    }

    hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
        std::wcerr << L"Failed to start Media Foundation" << std::endl;
        CoUninitialize();
        return 1;
    }

    std::vector<IMFActivate*> devices;
    hr = EnumerateAudioDevices(devices);
    if (FAILED(hr) || devices.empty()) {
        std::wcerr << L"Failed to enumerate audio devices" << std::endl;
        MFShutdown();
        CoUninitialize();
        return 1;
    }

    std::wcout << L"Available audio devices:" << std::endl;
    for (size_t i = 0; i < devices.size(); i++) {
        std::wstring friendlyName;
        hr = GetDeviceFriendlyName(devices[i], friendlyName);
        if (SUCCEEDED(hr)) {
            std::wcout << i << L": " << friendlyName << std::endl;
        }
    }

    size_t deviceIndex;
    std::wcout << L"Select a device (0-" << devices.size() - 1 << L"): ";
    std::wcin >> deviceIndex;

    if (deviceIndex >= devices.size()) {
        std::wcerr << L"Invalid device selection" << std::endl;
        for (auto& device : devices) device->Release();
        MFShutdown();
        CoUninitialize();
        return 1;
    }

    IMFMediaSource* pSource = NULL;
    hr = devices[deviceIndex]->ActivateObject(__uuidof(IMFMediaSource), (void**)&pSource);
    if (FAILED(hr)) {
        std::wcerr << L"Failed to activate media source" << std::endl;
        for (auto& device : devices) device->Release();
        MFShutdown();
        CoUninitialize();
        return 1;
    }

    std::vector<IMFMediaType*> profiles;
    hr = EnumerateProfiles(pSource, profiles);
    if (FAILED(hr) || profiles.empty()) {
        std::wcerr << L"Failed to enumerate profiles" << std::endl;
        pSource->Release();
        for (auto& device : devices) device->Release();
        MFShutdown();
        CoUninitialize();
        return 1;
    }

    std::wcout << L"Available profiles:" << std::endl;
    for (size_t i = 0; i < profiles.size(); i++) {
        std::wstring profileInfo;
        hr = GetAudioProfileInfo(profiles[i], profileInfo);
        if (SUCCEEDED(hr)) {
            std::wcout << i << L": " << profileInfo << std::endl;
        }
    }

    size_t profileIndex;
    std::wcout << L"Select a profile (0-" << profiles.size() - 1 << L"): ";
    std::wcin >> profileIndex;

    if (profileIndex >= profiles.size()) {
        std::wcerr << L"Invalid profile selection" << std::endl;
        for (auto& profile : profiles) profile->Release();
        pSource->Release();
        for (auto& device : devices) device->Release();
        MFShutdown();
        CoUninitialize();
        return 1;
    }

    std::wstring outputFileName;
    std::wcout << L"Enter output file name: ";
    std::wcin >> outputFileName;

    hr = RecordAudio(pSource, profiles[profileIndex], outputFileName);
    if (FAILED(hr)) {
        std::wcerr << L"Failed to record audio" << std::endl;
    }
    else {
        std::wcout << L"Audio recorded successfully to " << outputFileName << std::endl;
    }

    for (auto& profile : profiles) profile->Release();
    pSource->Release();
    for (auto& device : devices) device->Release();

    MFShutdown();
    CoUninitialize();

    return 0;
}