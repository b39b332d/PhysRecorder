#ifndef _CAMERADRIVER_V4L2_H_
#define _CAMERADRIVER_V4L2_H_

#include <string>
#include <CameraDriver.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <map>
#include <memory>

namespace capture {
    struct BufferInfo {
        void* start;
        size_t length;
    };
    class CameraProfileV4L2 : public CameraProfile {
    public:
        struct v4l2_frmivalenum v4l2_fmt;
        std::string stream_name;
        
        CameraProfileV4L2(const struct v4l2_frmivalenum& fmt, CameraStream* stream);
        ~CameraProfileV4L2();
    };

    class CameraStreamV4L2 : public CameraStream {
        void capture_thread_func();
    public:
    void force_stop();
        bool is_running = false;
        std::thread capture_thread;
        int fd = -1;
        CameraStreamV4L2(const std::string& stream_name, CameraDevice* device);
        ~CameraStreamV4L2();
        std::vector<BufferInfo> buffers;

        bool start();
        void stop();
        void uninit_mmap();
        bool init_mmap();
    };
    
    class CameraDeviceV4L2 : public CameraDevice {
        std::mutex r_lock;
        int running_streams=0;
    public:

        void on_stream_stop();
        CameraDeviceV4L2(const std::string& device_id);
        
        bool native_init() override;
        bool native_start() override;
        void native_stop() override;
        void native_release() override;
        
        void set_option_native(int option, const option_status& value) override;
        
    };
    
    std::vector<CameraDevice*> EnumerateCamera_V4L2();
}

#endif