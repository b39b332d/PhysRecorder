#pragma once
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

#include <iostream>
#include <qdebug>

const UINT WM_APP_PREVIEW_ERROR = WM_APP + 1;    // wparam = HRESULT

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
    HRESULT GetDevice(UINT32 index, IMFActivate** ppActivate);
    HRESULT GetDeviceName(UINT32 index, WCHAR** ppszName);
};




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
