#include "CameraDriverV4L2.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <dirent.h>
#include <errno.h>
#include <libv4l2.h>
#include <libudev.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <algorithm>
#include <regex>

namespace capture {

// Define the buffer count for memory mapping
#define V4L2_BUFFER_COUNT 4

#ifndef V4L2_IF_EQUAL_RETURN
#define V4L2_IF_EQUAL_RETURN(val) if(V4L2_PIX_FMT_##val == fmt) return PIX_TYPE_##val
#endif

PIX_TYPE V4L2_format_to_pixformat(uint32_t fmt) {
    V4L2_IF_EQUAL_RETURN(YUYV);
    V4L2_IF_EQUAL_RETURN(UYVY);
    V4L2_IF_EQUAL_RETURN(RGB24); // Same as RGB8
    if (V4L2_PIX_FMT_RGB24 == fmt) return PIX_TYPE_RGB8;
    if (V4L2_PIX_FMT_BGR24 == fmt) return PIX_TYPE_BGR8;
    if (V4L2_PIX_FMT_RGBA32 == fmt) return PIX_TYPE_RGBA;
    if (V4L2_PIX_FMT_ABGR32 == fmt) return PIX_TYPE_BGRA;
    if (V4L2_PIX_FMT_GREY == fmt) return PIX_TYPE_L8;
    if (V4L2_PIX_FMT_Y16 == fmt) return PIX_TYPE_L16;
    if (V4L2_PIX_FMT_Z16 == fmt) return PIX_TYPE_D16;
    if (V4L2_PIX_FMT_MJPEG == fmt) return PIX_TYPE_MJPG;
    if (V4L2_PIX_FMT_JPEG == fmt) return PIX_TYPE_JPEG;
    std::cout << "Unknown V4L2 format: 0x" << std::hex << fmt << std::dec << std::endl;
    return PIX_TYPE_ERR;
}

uint32_t pixformat_to_V4L2_format(PIX_TYPE format) {
    switch (format) {
        case PIX_TYPE_YUYV: return V4L2_PIX_FMT_YUYV;
        case PIX_TYPE_UYVY: return V4L2_PIX_FMT_UYVY;
        case PIX_TYPE_RGB8: return V4L2_PIX_FMT_RGB24;
        case PIX_TYPE_BGR8: return V4L2_PIX_FMT_BGR24;
        case PIX_TYPE_RGBA: return V4L2_PIX_FMT_RGBA32;
        case PIX_TYPE_BGRA: return V4L2_PIX_FMT_ABGR32;
        case PIX_TYPE_L8: return V4L2_PIX_FMT_GREY;
        case PIX_TYPE_L16: return V4L2_PIX_FMT_Y16;
        case PIX_TYPE_D16: return V4L2_PIX_FMT_Z16;
        case PIX_TYPE_MJPG: return V4L2_PIX_FMT_MJPEG;
        case PIX_TYPE_JPEG: return V4L2_PIX_FMT_JPEG;
        default: return 0;
    }
}

// Helper struct for buffer management
struct buffer {
    void* start;
    size_t length;
};

// Structure to store buffer mapping information
struct stream_buffer_info {
    int fd;
    buffer* buffers;
    unsigned int n_buffers;
    bool streaming;
};

// Map of device fd to buffer info
static std::unordered_map<int, stream_buffer_info> buffer_maps;

// Helper function to get device serial number using udev
std::string get_device_serial(const std::string& device_path) {
    struct udev* udev = udev_new();
    if (!udev) {
        return "";
    }

    struct stat st;
    if (stat(device_path.c_str(), &st) == -1) {
        udev_unref(udev);
        return "";
    }

    struct udev_device* dev = udev_device_new_from_devnum(udev, 'c', st.st_rdev);
    if (!dev) {
        udev_unref(udev);
        return "";
    }

    // Try to find a unique identifier for the device
    const char* serial = udev_device_get_property_value(dev, "ID_SERIAL");
    std::string result = serial ? serial : "";
    
    if (result.empty()) {
        // If no serial, try to use the path as a fallback identifier
        const char* syspath = udev_device_get_syspath(dev);
        if (syspath) {
            result = syspath;
        }
    }

    udev_device_unref(dev);
    udev_unref(udev);
    return result;
}

// Check if two devices belong to the same physical camera
bool is_same_physical_device(const std::string& path1, const std::string& path2) {
    std::string serial1 = get_device_serial(path1);
    std::string serial2 = get_device_serial(path2);
    
    // If we have valid serials, compare them
    if (!serial1.empty() && !serial2.empty()) {
        return serial1 == serial2;
    }
    
    // Fallback: Check if they're likely from the same device based on path pattern
    // Example: /dev/video0 and /dev/video1 might be from the same device
    std::regex video_pattern("/dev/video(\\d+)");
    std::smatch match1, match2;
    
    if (std::regex_match(path1, match1, video_pattern) && 
        std::regex_match(path2, match2, video_pattern)) {
        int num1 = std::stoi(match1[1].str());
        int num2 = std::stoi(match2[1].str());
        
        // Typically, cameras expose consecutive device nodes
        return std::abs(num1 - num2) == 1;
    }
    
    return false;
}

// Get a user-friendly device name
std::string get_device_name(const std::string& device_path) {
    int fd = v4l2_open(device_path.c_str(), O_RDWR);
    if (fd == -1) {
        return "Unknown V4L2 Device";
    }
    
    struct v4l2_capability cap;
    if (v4l2_ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1) {
        v4l2_close(fd);
        return "Unknown V4L2 Device";
    }
    
    std::string name = reinterpret_cast<const char*>(cap.card);
    v4l2_close(fd);
    return "V4L2: " + name;
}

// Initialize a buffer for streaming
bool init_mmap(int fd) {
    struct v4l2_requestbuffers req = {0};
    req.count = V4L2_BUFFER_COUNT;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    
    if (v4l2_ioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
        std::cerr << "Failed to initialize memory mapping: " << strerror(errno) << std::endl;
        return false;
    }
    
    if (req.count < 2) {
        std::cerr << "Insufficient buffer memory" << std::endl;
        return false;
    }
    
    stream_buffer_info& buffer_info = buffer_maps[fd];
    buffer_info.fd = fd;
    buffer_info.n_buffers = req.count;
    buffer_info.buffers = (buffer*)calloc(req.count, sizeof(buffer));
    buffer_info.streaming = false;
    
    for (unsigned int i = 0; i < req.count; ++i) {
        struct v4l2_buffer buf = {0};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        
        if (v4l2_ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
            std::cerr << "VIDIOC_QUERYBUF failed: " << strerror(errno) << std::endl;
            return false;
        }
        
        buffer_info.buffers[i].length = buf.length;
        buffer_info.buffers[i].start = v4l2_mmap(NULL, buf.length,
                                                PROT_READ | PROT_WRITE, MAP_SHARED,
                                                fd, buf.m.offset);
        
        if (buffer_info.buffers[i].start == MAP_FAILED) {
            std::cerr << "Memory mapping failed: " << strerror(errno) << std::endl;
            return false;
        }
    }
    
    return true;
}

// Clean up mapped memory
void uninit_mmap(int fd) {
    auto it = buffer_maps.find(fd);
    if (it == buffer_maps.end()) {
        return;
    }
    
    stream_buffer_info& buffer_info = it->second;
    for (unsigned int i = 0; i < buffer_info.n_buffers; ++i) {
        if (buffer_info.buffers[i].start != MAP_FAILED && buffer_info.buffers[i].start != nullptr) {
            v4l2_munmap(buffer_info.buffers[i].start, buffer_info.buffers[i].length);
            buffer_info.buffers[i].start = nullptr;
        }
    }
    
    free(buffer_info.buffers);
    buffer_info.buffers = nullptr;
    buffer_info.streaming = false;
    buffer_maps.erase(it);
}

// Start streaming
bool start_streaming(int fd) {
    auto it = buffer_maps.find(fd);
    if (it == buffer_maps.end() || it->second.streaming) {
        return false;
    }
    
    // Queue all buffers
    for (unsigned int i = 0; i < it->second.n_buffers; ++i) {
        struct v4l2_buffer buf = {0};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        
        if (v4l2_ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
            std::cerr << "VIDIOC_QBUF failed: " << strerror(errno) << std::endl;
            return false;
        }
    }
    
    // Start streaming
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (v4l2_ioctl(fd, VIDIOC_STREAMON, &type) == -1) {
        std::cerr << "VIDIOC_STREAMON failed: " << strerror(errno) << std::endl;
        return false;
    }
    
    it->second.streaming = true;
    return true;
}

// Stop streaming
bool stop_streaming(int fd) {
    auto it = buffer_maps.find(fd);
    if (it == buffer_maps.end() || !it->second.streaming) {
        return false;
    }
    
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (v4l2_ioctl(fd, VIDIOC_STREAMOFF, &type) == -1) {
        std::cerr << "VIDIOC_STREAMOFF failed: " << strerror(errno) << std::endl;
        return false;
    }
    
    it->second.streaming = false;
    return true;
}

// CameraStreamV4L2 implementation
CameraStreamV4L2::CameraStreamV4L2(const std::string& stream_name, const std::string& device_path,
                                 CameraDevice* device, int stream_index)
    : CameraStream(stream_name, device), 
      device_path(device_path), 
      stream_index(stream_index),
      fd(-1) {
    // Initialize with closed device
    memset(&v4l2_caps, 0, sizeof(v4l2_caps));
}

CameraStreamV4L2::~CameraStreamV4L2() {
    close_device();
}

bool CameraStreamV4L2::open_device() {
    if (fd >= 0) {
        // Already open
        return true;
    }
    
    // Open the device
    fd = v4l2_open(device_path.c_str(), O_RDWR);
    if (fd == -1) {
        std::cerr << "Cannot open device " << device_path << ": " << strerror(errno) << std::endl;
        return false;
    }
    
    // Get device capabilities
    if (v4l2_ioctl(fd, VIDIOC_QUERYCAP, &v4l2_caps) == -1) {
        std::cerr << "VIDIOC_QUERYCAP failed: " << strerror(errno) << std::endl;
        close_device();
        return false;
    }
    
    // Check if it's a capture device
    if (!(v4l2_caps.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        std::cerr << device_path << " is not a video capture device" << std::endl;
        close_device();
        return false;
    }
    
    // Check if it supports streaming
    if (!(v4l2_caps.capabilities & V4L2_CAP_STREAMING)) {
        std::cerr << device_path << " does not support streaming I/O" << std::endl;
        close_device();
        return false;
    }
    
    return true;
}

void CameraStreamV4L2::close_device() {
    if (fd >= 0) {
        auto it = buffer_maps.find(fd);
        if (it != buffer_maps.end() && it->second.streaming) {
            stop_streaming(fd);
        }
        
        uninit_mmap(fd);
        v4l2_close(fd);
        fd = -1;
    }
}

// CameraProfileV4L2 implementation
CameraProfileV4L2::CameraProfileV4L2(const v4l2_format& fmt, const v4l2_frmivalenum& interval,
                               CameraStream* stream)
    : CameraProfile(stream), v4l2_fmt(fmt), v4l2_frame_interval(interval) {
    
    format = V4L2_format_to_pixformat(fmt.fmt.pix.pixelformat);
    resolution.width = fmt.fmt.pix.width;
    resolution.height = fmt.fmt.pix.height;
    
    // Set up frame rate from interval (convert fraction to our ratio)
    if (interval.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
        ratio.numerator = interval.discrete.denominator;
        ratio.denominator = interval.discrete.numerator;
    } else {
        // Default to 30fps if interval type is not discrete
        ratio.numerator = 30;
        ratio.denominator = 1;
    }
    
    // Create a descriptive name for this profile
    format_name = std::string(GET_PIX_TYPE_NAME(format));
    
    // Build a descriptive stream name that includes the format
    stream_name = stream->stream_name + ":" + format_name;
}

CameraProfileV4L2::~CameraProfileV4L2() {
    // Nothing special to clean up
}

// CameraDeviceV4L2 implementation
CameraDeviceV4L2::CameraDeviceV4L2(const std::string& device_name, const std::string& device_serial)
    : device_serial(device_serial) {
    this->device_name = device_name;
}

CameraDeviceV4L2::~CameraDeviceV4L2() {
    native_release();
}

void CameraDeviceV4L2::add_stream(const std::string& device_path, int stream_index) {
    // Create a new stream for this device path
    std::string stream_name = "Stream " + std::to_string(stream_index);
    auto stream = std::make_shared<CameraStreamV4L2>(stream_name, device_path, this, stream_index);
    device_streams.push_back(stream);
}

bool CameraDeviceV4L2::native_init() {
    bool success = true;
    
    // Initialize each stream and collect its formats and resolutions
    for (auto& stream : device_streams) {
        if (!stream->open_device()) {
            success = false;
            continue;
        }
        
        int fd = stream->fd;
        if (fd < 0) {
            continue;
        }
        
        // Query available formats
        struct v4l2_fmtdesc fmt_desc;
        memset(&fmt_desc, 0, sizeof(fmt_desc));
        fmt_desc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        
        while (v4l2_ioctl(fd, VIDIOC_ENUM_FMT, &fmt_desc) == 0) {
            // For each format, query available frame sizes
            struct v4l2_frmsizeenum size_enum;
            memset(&size_enum, 0, sizeof(size_enum));
            size_enum.pixel_format = fmt_desc.pixelformat;
            
            while (v4l2_ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &size_enum) == 0) {
                if (size_enum.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                    // For each resolution, query available frame rates
                    struct v4l2_frmivalenum interval_enum;
                    memset(&interval_enum, 0, sizeof(interval_enum));
                    interval_enum.pixel_format = fmt_desc.pixelformat;
                    interval_enum.width = size_enum.discrete.width;
                    interval_enum.height = size_enum.discrete.height;
                    
                    while (v4l2_ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &interval_enum) == 0) {
                        // Create a format structure for this combination
                        struct v4l2_format format;
                        memset(&format, 0, sizeof(format));
                        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        format.fmt.pix.width = size_enum.discrete.width;
                        format.fmt.pix.height = size_enum.discrete.height;
                        format.fmt.pix.pixelformat = fmt_desc.pixelformat;
                        
                        // Create a profile for this format/resolution/framerate
                        CameraProfile* profile = new CameraProfileV4L2(format, interval_enum, stream.get());
                        
                        // Add this profile to the stream
                        if (profile->is_valid()) {
                            stream->profiles_map[GET_PIX_TYPE_NAME(profile->format)].insert(profile);
                            
                            // If no default profile yet, set this as default
                            if (stream->default_profile == nullptr) {
                                stream->default_profile = profile;
                            }
                        } else {
                            delete profile;
                        }
                        
                        interval_enum.index++;
                    }
                }
                size_enum.index++;
            }
            fmt_desc.index++;
        }
        
        // Add this stream to our streams map
        CameraStream*& map_stream = streams_map[stream->stream_name];
        if (map_stream == nullptr) {
            map_stream = stream.get();
        }
    }
    
    // Load all the camera controls/options
    get_all_option_range_native();
    
    return success;
}

bool CameraDeviceV4L2::native_start() {
    if (enabled_streams.empty()) {
        return false;
    }
    
    // For each enabled stream, set up and start capturing
    bool all_started = true;
    
    for (auto stream_ptr : enabled_streams) {
        CameraStreamV4L2* stream = static_cast<CameraStreamV4L2*>(stream_ptr);
        if (stream->fd < 0 && !stream->open_device()) {
            all_started = false;
            continue;
        }
        
        int fd = stream->fd;
        CameraProfileV4L2* profile = static_cast<CameraProfileV4L2*>(stream->get_current_profile());
        
        // Set the format
        struct v4l2_format fmt = profile->v4l2_fmt;
        if (v4l2_ioctl(fd, VIDIOC_S_FMT, &fmt) == -1) {
            std::cerr << "VIDIOC_S_FMT failed: " << strerror(errno) << std::endl;
            all_started = false;
            continue;
        }
        
        // Set the frame rate
        struct v4l2_streamparm parm;
        memset(&parm, 0, sizeof(parm));
        parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        parm.parm.capture.timeperframe.numerator = profile->ratio.denominator;
        parm.parm.capture.timeperframe.denominator = profile->ratio.numerator;
        
        if (v4l2_ioctl(fd, VIDIOC_S_PARM, &parm) == -1) {
            std::cerr << "VIDIOC_S_PARM failed: " << strerror(errno) << std::endl;
            // Not critical, continue anyway
        }
        
        // Set up memory mapping
        if (!init_mmap(fd)) {
            all_started = false;
            continue;
        }
        
        // Start the streaming
        if (!start_streaming(fd)) {
            all_started = false;
            continue;
        }
        
        // Create a capture thread for this stream
        std::shared_ptr<CameraDeviceV4L2> self_ref(this, [](CameraDeviceV4L2* device) {
            device->onDeviceReadingFailed();
        });
        
        std::thread capture_thread([this, self_ref, stream, fd]() {
            // Calculate timestamps
            long long time_offset = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            
            while (true) {
                // Check if we should continue running
                device_lock.lock();
                if (status != CameraDevice::CS_RUNNING) {
                    device_lock.unlock();
                    break;
                }
                device_lock.unlock();
                
                // Wait for a frame with timeout
                fd_set fds;
                FD_ZERO(&fds);
                FD_SET(fd, &fds);
                
                struct timeval tv;
                tv.tv_sec = 1;
                tv.tv_usec = 0;
                
                int r = select(fd + 1, &fds, NULL, NULL, &tv);
                if (r == -1) {
                    if (errno == EINTR) {
                        continue;
                    }
                    break;
                }
                
                if (r == 0) {
                    // Timeout
                    continue;
                }
                
                // Read the frame
                struct v4l2_buffer buf;
                memset(&buf, 0, sizeof(buf));
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                
                if (v4l2_ioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
                    if (errno == EAGAIN) {
                        continue;
                    }
                    break;
                }
                
                // Get the buffer info
                auto it = buffer_maps.find(fd);
                if (it == buffer_maps.end()) {
                    break;
                }
                
                // Get the current timestamp
                long long curr_ts = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                
                // Create a frame with the buffer data
                unsigned char* buffer_ptr = static_cast<unsigned char*>(it->second.buffers[buf.index].start);
                size_t buffer_size = buf.bytesused;
                
                // Create a capture frame
                CameraProfile* profile = stream->get_current_profile();
                if (profile) {
                    // Create a copy of the frame data
                    unsigned char* frame_copy = new unsigned char[buffer_size];
                    memcpy(frame_copy, buffer_ptr, buffer_size);
                    
                    // Create the frame object
                    RawFrame* frame = profile->createFrame(
                        curr_ts - time_offset,
                        frame_copy,
                        buffer_size,
                        [frame_copy]() { delete[] frame_copy; }
                    );
                    
                    // Add the frame to the stream's queue
                    stream->write(frame);
                }
                
                // Re-queue the buffer
                if (v4l2_ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
                    break;
                }
            }
            
            // Stop streaming on