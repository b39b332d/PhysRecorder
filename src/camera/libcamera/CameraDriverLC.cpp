#include "CameraDriverLC.h"

#include <atomic>
#include <iomanip>
#include <iostream>
#include <signal.h>
#include <limits.h>
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <sstream>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>
#include <mutex>

#include <libcamera/controls.h>
#include <libcamera/control_ids.h>
#include <libcamera/property_ids.h>
#include <libcamera/libcamera.h>
#include <libcamera/camera.h>
#include <libcamera/camera_manager.h>
#include <libcamera/framebuffer_allocator.h>
#include <libcamera/request.h>
#include <libcamera/stream.h>
#include <libcamera/formats.h>
#include <libcamera/transform.h>

namespace capture {
    CameraProfileLC::CameraProfileLC(CameraStream* stream)
        : CameraProfile(stream)
    {
    }

    CameraProfileLC::~CameraProfileLC()
    {
    }

    // Implementation of CameraStreamV4L2
    CameraStreamLC::CameraStreamLC(libcamera::Stream* stream, const std::string& stream_name, CameraDevice* device)
        : CameraStream(stream_name, device),stream(stream)
    {
        auto stream_fmts = stream->configuration().formats();
        for(auto pix_fmt : stream_fmts.pixelformats()){

            PIX_TYPE pix_type = (PIX_TYPE)(pix_fmt.fourcc());
            auto format_name = GET_PIX_TYPE_NAME(pix_type);
            
            ProfileSet prof_set;
            for (auto& size:stream_fmts.sizes(pix_fmt)) {
                CameraProfileLC *prof = new CameraProfileLC(this);
                prof->format = pix_type;
                prof->resolution.width = size.width;
                prof->resolution.height = size.height;
                prof->ratio.denominator = 1;
                prof->ratio.numerator = 0;
                prof_set.insert(prof);
            }
            profiles_map[format_name] = std::move(prof_set);
        }
    }

    CameraStreamLC::~CameraStreamLC()
    {
    }

    libcamera::CameraManager* cm = nullptr;
    std::vector<CameraDevice*> EnumerateCamera_LC()
    {
        if(cm == nullptr){
            cm = new libcamera::CameraManager;
            cm->start();
        }
        std::vector<CameraDevice*> devices;
        for (auto const &camera : cm->cameras()){
            CameraDevice* device = new CameraDeviceLC(camera->id());
            devices.emplace_back(device);
        }
        return devices;
    }

    CameraDeviceLC::CameraDeviceLC(const std::string &device_id)
    {
        camera = cm->get(device_id);
        device_name = "LC: "+device_id;
    }
    bool CameraDeviceLC::native_init()
    {
        if(camera->acquire()<0) return false;
        auto &streams = camera->streams();
        for (libcamera::Stream *lc_stream : streams) {
             CameraStreamLC* stream = new CameraStreamLC(lc_stream,lc_stream->configuration().toString(),this);
             if(stream->is_valid())
                streams_map[stream->stream_name] = stream;
            else
                delete stream;
        }
        if(streams_map.empty()) return false;
        get_all_option_range_native();
        return true;
    }
    
    bool CameraDeviceLC::native_start() {

        return true;
    };

    void CameraDeviceLC::native_stop() {

    };
    void CameraDeviceLC::native_release() {
        camera->release();
        
    };
        
    void CameraDeviceLC::get_all_option_range_native(){

    }
    void CameraDeviceLC::set_option_native(int option, const option_status& value){
    }
}
