#ifndef _CAMERADRIVER_V4L2_H_
#define _CAMERADRIVER_V4L2_H_

#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <memory>
#include <linux/videodev2.h>
#include "CameraDriver.h"

namespace capture {

class CameraDeviceV4L2;

// Represents a V4L2 video device (/dev/video*)
class CameraStreamV4L2 : public CameraStream {
public:
    int fd; // File descriptor for the /dev/video device
    std::string device_path; // Path to the device (e.g., /dev/video0)
    int stream_index; // Index for multiple streams from same physical device
    struct v4l2_capability v4l2_caps; // Device capabilities

    CameraStreamV4L2(const std::string& stream_name, const std::string& device_path, 
                     CameraDevice* device, int stream_index);
    ~CameraStreamV4L2();

    bool open_device();
    void close_device();
};

// Represents a single physical camera that might expose multiple V4L2 devices
class CameraProfileV4L2 : public CameraProfile {
public:
    v4l2_format v4l2_fmt;
    v4l2_frmivalenum v4l2_frame_interval;
    std::string format_name;

    CameraProfileV4L2(const v4l2_format& fmt, const v4l2_frmivalenum& interval, 
                      CameraStream* stream);
    ~CameraProfileV4L2();
};

// Collection of V4L2 devices that belong to the same physical camera
class CameraDeviceV4L2 : public CameraDevice {
private:
    std::string device_serial; // Unique identifier for the physical device
    std::vector<std::shared_ptr<CameraStreamV4L2>> device_streams; // All V4L2 streams from this device
    
    // Helper methods for option handling
    void set_single_option_native(int option, const option_status& value);
    bool get_control(int fd, uint32_t id, int32_t& value);
    bool set_control(int fd, uint32_t id, int32_t value);
    
public:
    CameraDeviceV4L2(const std::string& device_name, const std::string& device_serial);
    ~CameraDeviceV4L2();
    
    // Add a V4L2 device to this physical camera
    void add_stream(const std::string& device_path, int stream_index);
    
    // Implementation of parent class virtual methods
    bool native_init() override;
    bool native_start() override;
    void native_stop() override;
    void native_release() override;
    
    // Options handling
    void get_all_option_range_native();
    void set_option_native(int option, const option_status& value) override;
};

// Helper functions
bool is_same_physical_device(const std::string& path1, const std::string& path2);
std::string get_device_serial(const std::string& device_path);
PIX_TYPE V4L2_format_to_pixformat(uint32_t v4l2_format);
uint32_t pixformat_to_V4L2_format(PIX_TYPE format);
std::string get_device_name(const std::string& device_path);

// Main function to enumerate V4L2 cameras
std::vector<CameraDevice*> EnumerateCamera_V4L2();

}

#endif