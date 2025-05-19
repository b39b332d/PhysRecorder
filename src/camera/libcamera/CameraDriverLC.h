#ifndef _CAMERADRIVER_V4L2_H_
#define _CAMERADRIVER_V4L2_H_

#include <string>
#include <CameraDriver.h>
#include <unistd.h>
#include <map>
#include <memory>
#include<libcamera/libcamera.h>

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
    class CameraDeviceLC;

    class CameraStreamLC : public CameraStream {
        libcamera::StreamConfiguration& stream_conf;
        libcamera::StreamRole stream_type;
	    std::map<libcamera::FrameBuffer *, std::vector<libcamera::Span<uint8_t>>> mapped_buffers_;
	    std::queue<libcamera::FrameBuffer *> frame_buffers_;

        // temp variables
        int index;
        libcamera::CameraConfiguration* conf;
    public:
        libcamera::Stream* stream;
        CameraStreamLC(libcamera::StreamConfiguration& stream_conf,libcamera::StreamRole stream_type, CameraDeviceLC* device);
        ~CameraStreamLC();

        void start(libcamera::CameraConfiguration* conf);
        void alloc_buf(libcamera::FrameBufferAllocator *allocator);
        bool make_request(libcamera::Request*);
        void stop();
        bool request_complete(libcamera::FrameBuffer *,libcamera::Request* request);
    };
    
    class CameraDeviceLC : public CameraDevice {
        std::map<libcamera::Stream *,CameraStreamLC*> lc_stream_map;
        std::map<libcamera::StreamRole,std::unique_ptr<libcamera::CameraConfiguration>> stream_conf_map;
        
	    libcamera::ControlList controls_;
        libcamera::Request* request;
    public:
        std::shared_ptr<libcamera::Camera> camera;
        libcamera::FrameBufferAllocator *allocator;
        CameraDeviceLC(const std::string& device_id);
        
        bool native_init() override;
        bool native_start() override;
        void native_stop() override;
        void native_release() override;
        
        void set_option_native(int option, const option_status& value) override;
        void get_all_option_range_native();
        void requestComplete(libcamera::Request *request);
        
    };
    
    std::vector<CameraDevice*> EnumerateCamera_LC();
}

#endif