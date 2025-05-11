#ifndef _CAMERADRIVER_V4L2_H_
#define _CAMERADRIVER_V4L2_H_

#include <string>
#include <CameraDriver.h>
#include <unistd.h>
#include <map>
#include <memory>
namespace libcamera{
    class Camera;
    class Stream;
    class StreamConfiguration;
}
namespace capture {
    struct BufferInfo {
        void* start;
        size_t length;
    };
    class CameraProfileLC : public CameraProfile {
    public:
        CameraProfileLC(CameraStream* stream);
        ~CameraProfileLC();
    };

    class CameraStreamLC : public CameraStream {
        libcamera::Stream* stream;
    public:
        CameraStreamLC(libcamera::Stream* stream,const std::string& stream_name, CameraDevice* device);
        ~CameraStreamLC();
    };
    
    class CameraDeviceLC : public CameraDevice {
        static std::shared_ptr<libcamera::Camera> camera;
    public:
        CameraDeviceLC(const std::string& device_id);
        
        bool native_init() override;
        bool native_start() override;
        void native_stop() override;
        void native_release() override;
        
        void set_option_native(int option, const option_status& value) override;
        void get_all_option_range_native();
        
    };
    
    std::vector<CameraDevice*> EnumerateCamera_LC();
}

#endif