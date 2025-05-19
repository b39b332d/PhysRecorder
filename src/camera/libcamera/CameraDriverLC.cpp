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
#include<libcamera/version.h>



namespace capture {
    
    CameraProfileLC::CameraProfileLC(CameraStream* stream)
        : CameraProfile(stream)
    {
    }

    CameraProfileLC::~CameraProfileLC()
    {
    }

    #if LIBCAMERA_VERSION_MAJOR ==0 &&  LIBCAMERA_VERSION_MINOR <=3
    static std::map<libcamera::StreamRole,std::string> stream_name_map = {
        std::make_pair(libcamera::StreamRole::VideoRecording,"VideoRecording"),
        std::make_pair(libcamera::StreamRole::Viewfinder,"Viewfinder"),
        std::make_pair(libcamera::StreamRole::StillCapture,"StillCapture"),
        std::make_pair(libcamera::StreamRole::StillCaptureRaw,"StillCaptureRaw")};
    #else
    static std::map<libcamera::StreamRole,std::string> stream_name_map = {
        std::make_pair(libcamera::StreamRole::VideoRecording,"VideoRecording"),
        std::make_pair(libcamera::StreamRole::Viewfinder,"Viewfinder"),
        std::make_pair(libcamera::StreamRole::StillCapture,"StillCapture"),
        std::make_pair(libcamera::StreamRole::Raw,"Raw")};
    #endif
    // Implementation of CameraStreamV4L2
    CameraStreamLC::CameraStreamLC(libcamera::StreamConfiguration& stream_conf,libcamera::StreamRole stream_type, CameraDeviceLC* device)
        : CameraStream("", device),stream_type(stream_type),stream_conf(stream_conf)
    {
        auto stream_fmts = stream_conf.formats();
        CameraProfile* temp = nullptr;
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

                if(size == stream_conf.size && pix_fmt == stream_conf.pixelFormat ){
                    default_profile = prof;
                }
                if(temp == nullptr)temp = prof;
            }
            profiles_map[format_name] = std::move(prof_set);
        }
        if(default_profile == nullptr)default_profile = temp;
        
    }

    CameraStreamLC::~CameraStreamLC()
    {
    }

    void CameraStreamLC::start(libcamera::CameraConfiguration* conf)
    {
        stream_conf.pixelFormat = libcamera::PixelFormat(get_current_profile()->format);
        stream_conf.size.width  = get_current_profile()->resolution.width;
        stream_conf.size.height = get_current_profile()->resolution.height;
        stream_conf.bufferCount = 4;
        // if(conf == nullptr){
        //      conf= ((CameraDeviceLC*)device)->camera->generateConfiguration({stream_type});
        // }
        conf->addConfiguration(stream_conf);
        index = conf->size()-1;
        this->conf = conf;
    }


    static libcamera::CameraManager* cm = nullptr;
    static std::string cm_version;
    std::vector<CameraDevice*> EnumerateCamera_LC()
    {
        if(cm == nullptr){
            cm = new libcamera::CameraManager;
            cm->start();
            cm_version = cm->version();
        }
        std::vector<CameraDevice*> devices;
        for (auto const &camera : cm->cameras()){
            #if LIBCAMERA_VERSION_MAJOR ==0 &&  LIBCAMERA_VERSION_MINOR <=3
            CameraDevice* device = new CameraDeviceLC(camera->name());
            #else
            CameraDevice* device = new CameraDeviceLC(camera->id());
            #endif
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

        for(auto &[stream_type,stream_name] : stream_name_map){
            std::unique_ptr<libcamera::CameraConfiguration> confs = camera->generateConfiguration({stream_type});
            if(!confs || confs->empty())
                continue;
            for (libcamera::StreamConfiguration &stream_conf : *confs) {
                CameraStreamLC* stream = new CameraStreamLC(stream_conf,stream_type,this);
                if(stream->is_valid()){
                    streams_map[stream_name] = stream;
                }
                else
                    delete stream;
            }
            stream_conf_map[stream_type]  =std::move(confs);
        }

        if(streams_map.empty()) return false;
        get_all_option_range_native();
        return true;
    }
    
    bool CameraDeviceLC::native_start() {
        auto conf = camera->generateConfiguration({});
        for (auto& s : enabled_streams) {
            ((CameraStreamLC*)s)->start(conf.get());
        }
        
        auto o = conf->validate();
        camera->configure(conf.get());
        allocator = new libcamera::FrameBufferAllocator(camera);

        for (auto& s : enabled_streams) {
            ((CameraStreamLC*)s)->alloc_buf(allocator);
            lc_stream_map[((CameraStreamLC*)s)->stream] = ((CameraStreamLC*)s);
        }
        request = camera->createRequest();

        for (auto& s : enabled_streams) {
            ((CameraStreamLC*)s)->make_request(request);
        }
        if (camera->start())
            throw std::runtime_error("failed to start camera");

        camera->requestCompleted.connect(this, &CameraDeviceLC::requestComplete);

        if (camera->queueRequest(request) < 0)
            throw std::runtime_error("Failed to queue request");

        return true;;
    };

    void CameraDeviceLC::native_stop() {
        delete allocator;
    };
    void CameraDeviceLC::native_release() {
        delete allocator;
        camera->release();
        stream_conf_map.clear();

        lc_stream_map.clear();
        
    };
    void CameraStreamLC::stop() {
        for (auto &iter : mapped_buffers_)
        {
            for (auto &span : iter.second)
                munmap(span.data(), span.size());
        }
        while(frame_buffers_.size()!=0){
            frame_buffers_.pop();
        }
	    
    }
        
    void CameraStreamLC::alloc_buf(libcamera::FrameBufferAllocator *allocator)
    {
        stream = conf->at(index).stream();

        if (allocator->allocate(stream) < 0)
            throw std::runtime_error("failed to allocate capture buffers");

        for (const std::unique_ptr<libcamera::FrameBuffer> &buffer : allocator->buffers(stream))
        {
            // "Single plane" buffers appear as multi-plane here, but we can spot them because then
            // planes all share the same fd. We accumulate them so as to mmap the buffer only once.
            size_t buffer_size = 0;
            for (unsigned i = 0; i < buffer->planes().size(); i++)
            {
                const libcamera::FrameBuffer::Plane &plane = buffer->planes()[i];
                buffer_size += plane.length;
                #if LIBCAMERA_VERSION_MAJOR ==0 &&  LIBCAMERA_VERSION_MINOR <=3
                int fd = plane.fd.fd();
                if (i == buffer->planes().size() - 1 || fd != buffer->planes()[i + 1].fd.fd())
                #else
                int fd = plane.fd.get();
                if (i == buffer->planes().size() - 1 || fd != buffer->planes()[i + 1].fd.get())
                #endif
                {
                    void *memory = mmap(NULL, buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                    mapped_buffers_[buffer.get()].push_back(
                        libcamera::Span<uint8_t>(static_cast<uint8_t *>(memory), buffer_size));
                    buffer_size = 0;
                }
            }
            frame_buffers_.push(buffer.get());
        }
    }
    
    bool CameraStreamLC::make_request(libcamera::Request* request){
        if(frame_buffers_.size()==0)return false;
        libcamera::FrameBuffer *buffer = frame_buffers_.front();
        frame_buffers_.pop();
        if (request->addBuffer(stream, buffer) < 0)
            throw std::runtime_error("failed to add buffer to request");
        return true;
    }
    
    bool CameraStreamLC::request_complete(libcamera::FrameBuffer *buffer,libcamera::Request* request){
        CameraProfile* profile = get_current_profile();
        if (profile) {
            // Get the mapped memory for this buffer
            auto it = mapped_buffers_.find(buffer);
            if (it != mapped_buffers_.end()) {
                // Create a timestamp using the buffer metadata
                uint64_t timestamp = buffer->metadata().timestamp;
                
                // Compute total buffer size from spans
                size_t total_size = 0;
                for (auto &span : it->second) {
                    total_size += span.size();
                }
                
                
                // Create a copy of the buffer data
                unsigned char* data = new unsigned char[total_size];
                size_t offset = 0;
                for (auto &span : it->second) {
                    memcpy(data + offset, span.data(), span.size());
                    offset += span.size();
                }
                bool ret = make_request(request);
                
                RawFrame* frame = profile->createFrame(
                    timestamp,
                    data,
                    total_size,
                    [data,buffer,this]() {
                        frame_buffers_.push(buffer);
                        delete[] data;
                    }
                );
                write(frame);
                return ret;
            }
        }
        return false;
    }
    void CameraDeviceLC::requestComplete(libcamera::Request *request)
    {
        if (request->status() == libcamera::Request::RequestCancelled)
            return;

        libcamera::Request* new_req =camera->createRequest();
        // Extract buffer information and create a RawFrame
        for (auto &buffer_pair : request->buffers()) {
            libcamera::Stream *stream = buffer_pair.first;
            libcamera::FrameBuffer *buffer = buffer_pair.second;
            
            // Get the current profile to create the frame
            CameraStreamLC* c_stream = lc_stream_map[stream];
            c_stream->request_complete(buffer,new_req);
        }
        
        camera->queueRequest(new_req);
        this->request = std::move(new_req);
    }
    void CameraDeviceLC::get_all_option_range_native(){

    }
    void CameraDeviceLC::set_option_native(int option, const option_status& value){
    }
}
