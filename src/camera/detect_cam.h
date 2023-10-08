#ifndef _DETECT_CAM_H
#define _DETECT_CAM_H
#include <memory>
#include <vector>
#include <windows.h>
std::shared_ptr<std::vector<WCHAR*>> GetDeviceList();
#endif