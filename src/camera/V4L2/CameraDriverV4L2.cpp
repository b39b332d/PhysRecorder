#include "CameraDriverV4L2.h"

#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <iostream>
#include <algorithm>
#include <sys/mman.h>
#include <chrono>
#include <regex>
#include <fstream>
#include <csignal>
#include <pthread.h>
#include <sstream>

namespace capture {
    static PIX_TYPE fmt2pix(unsigned int fmt){
        if(fmt == V4L2_PIX_FMT_YUYV)
            return PIX_TYPE_YUY2;
        else
            return *((PIX_TYPE*)&fmt);
    }
    static unsigned int pix2fmt(PIX_TYPE fmt){
        if(fmt ==  PIX_TYPE_YUY2)
            return V4L2_PIX_FMT_YUYV ;
        else
            return *((unsigned int*)&fmt);
    }
    // Implementation of CameraProfileV4L2
    CameraProfileV4L2::CameraProfileV4L2(const struct v4l2_frmivalenum& fmt,  CameraStream* stream)
        : CameraProfile(stream)
    {
        resolution = {fmt.width,fmt.height};
        format = fmt2pix(fmt.pixel_format);
        ratio =  {fmt.discrete.denominator,fmt.discrete.numerator}; // interval to fps
    }

    CameraProfileV4L2::~CameraProfileV4L2()
    {
    }

    // Implementation of CameraStreamV4L2
    CameraStreamV4L2::CameraStreamV4L2(const std::string& stream_name, CameraDevice* device)
        : CameraStream(stream_name, device)
    {
        const int fd = open(stream_name.c_str(), O_RDWR);
        if (fd <0) return;
        struct v4l2_fmtdesc fmt;
        struct v4l2_frmsizeenum frmsize;
        struct v4l2_frmivalenum frmival;
        fmt.index = 0;
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        while (ioctl(fd, VIDIOC_ENUM_FMT, &fmt) >= 0) {
            auto pix_type = fmt2pix(fmt.pixelformat);
            auto format_name = GET_PIX_TYPE_NAME(pix_type);

            frmsize.pixel_format = fmt.pixelformat;
            frmsize.index = 0;
            ProfileSet prof_set;
            while (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) >= 0) {
                if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                    frmival.index = 0;
                    frmival.pixel_format = fmt.pixelformat;
                    frmival.width = frmsize.discrete.width;
                    frmival.height = frmsize.discrete.height;
                    while (ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) >= 0) {
                        CameraProfileV4L2* prof = new CameraProfileV4L2(frmival,this);
                        prof_set.insert(prof);
                        if(default_profile == nullptr)default_profile = prof;
                        frmival.index++;
                    }
                }
                frmsize.index++;
            }
            fmt.index++;
            profiles_map[format_name] = std::move(prof_set);
        }
        close(fd);
    }

    CameraStreamV4L2::~CameraStreamV4L2()
    {
    }


    std::vector<CameraDevice*> EnumerateCamera_V4L2()
    {
        std::vector<CameraDevice*> devices;
        std::unordered_map<std::string, CameraDeviceV4L2*> device_map;
        std::vector<std::string> files;
 
        const std::string dev_folder = "/dev/";

        DIR *dir;
        struct dirent *ent;
        if ((dir = opendir(dev_folder.c_str())) != NULL)
        {
            while ((ent = readdir(dir)) != NULL)
            {
                if (strlen(ent->d_name) > 5 && !strncmp("video", ent->d_name, 5)) {
                    
                    std::string file = dev_folder + ent->d_name;

                    const int fd = open(file.c_str(), O_RDWR);
                    v4l2_capability vcap = {0,};
                    if (fd >= 0) {
                        int err = ioctl(fd, VIDIOC_QUERYCAP, &vcap);
                        close(fd);
                        std::string bus_info;
                        std::string card;
                        if (err == 0)
                        {
                            bus_info = reinterpret_cast<const char *>(vcap.bus_info);
                            card = reinterpret_cast<const char *>(vcap.card);
                        }

                        if (!bus_info.empty() && !card.empty())
                        {
                            auto pdev = device_map.find(bus_info); 
                            CameraDeviceV4L2* device;
                            if (pdev != device_map.end())
                            {
                                device = pdev->second;
                            }
                            else
                            {
                                device = new CameraDeviceV4L2(card + " (" + bus_info + ")");
                                device_map[bus_info] = device;
                                devices.push_back(device);
                            }
                            device->streams_map[file] = nullptr;

                        }
                    }
                }
            }
            closedir(dir);
        }
        return devices;
    }

    CameraDeviceV4L2::CameraDeviceV4L2(const std::string &device_id)
    {
        device_name =device_id;
    }
    bool CameraDeviceV4L2::native_init()
    {

        for (auto it = streams_map.begin(); it != streams_map.end();) {
            auto &stream = it->second;
            if(stream == nullptr){
                stream = new CameraStreamV4L2(it->first,this);
                if(!stream->is_valid()){
                    it = streams_map.erase(it);
                }else
                ++it;
            }
        }
        return true;
    }
    
    bool CameraDeviceV4L2::native_start() {
        std::vector<CameraStreamV4L2*> started;
        for (auto& s : enabled_streams) {
            if(((CameraStreamV4L2*)s)->start()){
                running_streams++;
                started.push_back((CameraStreamV4L2*)s);
            }else{
                for(auto s :started){
                    s->force_stop();
                }
                running_streams=0;
                return false;
            }
        }
        return true;
    };
    
    bool CameraStreamV4L2::init_mmap()
    {
        struct v4l2_requestbuffers req;
        memset(&req, 0, sizeof(req));
        req.count = 4;  // Request 4 buffers
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;
        
        if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
            std::cerr << "Failed to request buffers: " << strerror(errno) << std::endl;
            return false;
        }
        
        if (req.count < 2) {
            std::cerr << "Insufficient buffer memory " << std::endl;
            return false;
        }
        
        // Allocate buffer info
        buffers.resize(req.count);
        
        for (unsigned int i = 0; i < req.count; i++) {
            struct v4l2_buffer buf;
            memset(&buf, 0, sizeof(buf));
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;
            
            if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
                std::cerr << "Failed to query buffer: " << strerror(errno) << std::endl;
                return false;
            }
            
            buffers[i].length = buf.length;
            buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
            
            if (buffers[i].start == MAP_FAILED) {
                std::cerr << "Failed to map buffer: " << strerror(errno) << std::endl;
                // Unmap already mapped buffers
                for (unsigned int j = 0; j < i; j++) {
                    munmap(buffers[j].start, buffers[j].length);
                }
                return false;
            }
        }
        
        return true;
    }

    void CameraStreamV4L2::uninit_mmap()
    {
        for (auto& buffer : buffers) {
            if (buffer.start != MAP_FAILED && buffer.start != nullptr) {
                munmap(buffer.start, buffer.length);
                buffer.start = nullptr;
            }
        }
        
        buffers.clear();
        
        // Release buffers from device
        struct v4l2_requestbuffers req;
        memset(&req, 0, sizeof(req));
        req.count = 0;  // Free all buffers
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;
        
        ioctl(fd, VIDIOC_REQBUFS, &req);  // Ignore errors
    }

    bool CameraStreamV4L2::start(){
        fd = open(stream_name.c_str(), O_RDWR);
        
        CameraProfileV4L2* profile = static_cast<CameraProfileV4L2*>(get_current_profile());
        
        // Set the format
        struct v4l2_format fmt={0,};
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width = profile->resolution.width;
        fmt.fmt.pix.height = profile->resolution.height;
        fmt.fmt.pix.pixelformat = pix2fmt(profile->format);

        fmt.fmt.pix.field = V4L2_FIELD_ANY;

        if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
            return false;
        }
        
        // Set the frame interval (FPS)
        struct v4l2_streamparm parm={0,};
        parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        parm.parm.capture.timeperframe.numerator = profile->ratio.denominator;
        parm.parm.capture.timeperframe.denominator = profile->ratio.numerator;
        ioctl(fd, VIDIOC_S_PARM, &parm);


        
        // Initialize memory mapping
        if (!init_mmap()) {
            return false;
        }
        
        // Queue all buffers
        for (unsigned int i = 0; i < buffers.size(); i++) {
            struct v4l2_buffer buf;
            memset(&buf, 0, sizeof(buf));
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;
            
            if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
                std::cerr << "Failed to queue buffer: " << strerror(errno) << std::endl;
                uninit_mmap();
                return false;
            }
        }
        
        // Start streaming
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
            std::cerr << "Failed to start streaming: " << strerror(errno) << std::endl;
            uninit_mmap();
            return false;
        }
        is_running = true;
        // Start the capture thread
        capture_thread = std::thread(&CameraStreamV4L2::capture_thread_func, this);
        return true;
        
    }
    void CameraStreamV4L2::stop(){
        if(!is_running) return;
        is_running = false;
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(fd, VIDIOC_STREAMOFF, &type) < 0) {
            std::cerr << "Failed to stop streaming : " << strerror(errno) << std::endl;
        }
        // Uninitialize memory mapping
        uninit_mmap();
        close(fd);
    }
    void CameraStreamV4L2::force_stop(){
        if(!is_running) return;
        pthread_kill(capture_thread.native_handle(),SIGUSR1);
        if (capture_thread.joinable()) {
            capture_thread.join();
        }
        stop();
    }
    
    void CameraStreamV4L2::capture_thread_func()
    {
        struct sigaction sa={0,};
        sa.sa_handler = [](int sig_n){};
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGUSR1, &sa, nullptr);

        struct v4l2_buffer buf;
        
        while (is_running) {
            // Dequeue a buffer
            memset(&buf, 0, sizeof(buf));
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            
            if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
                if (errno == EAGAIN) {
                    // No buffer ready, try again
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    continue;
                }
                
                break;
            }
            
            // Get the timestamp
            long long timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            
            // Make a copy of the frame data to avoid issues when the buffer is reused
            unsigned char* data = static_cast<unsigned char*>(buffers[buf.index].start);
            size_t size = buf.bytesused;
            
            // Create a frame and write it to the stream
            CameraProfile* profile = get_current_profile();
            if (profile) {
                RawFrame* frame = profile->createFrame(
                    timestamp,
                    data,
                    size,
                    [this,buf]() { ioctl(fd, VIDIOC_QBUF, &buf); }
                );
                
                write(frame);
            }
            
        }
        stop();
        ((CameraDeviceV4L2*)device)->on_stream_stop();
    }

    void CameraDeviceV4L2::on_stream_stop(){
        r_lock.lock();
        running_streams--;
        if(running_streams ==0){
            r_lock.unlock();
            onDeviceReadingFailed();
        }else
        r_lock.unlock();
    }

    void CameraDeviceV4L2::native_stop() {
        auto t = enabled_streams;
        for (auto& s : t) {
            ((CameraStreamV4L2*)s)->force_stop();
        }
    };
    void CameraDeviceV4L2::native_release() {};
    
    void CameraDeviceV4L2::set_option_native(int option, const option_status& value) {};
}