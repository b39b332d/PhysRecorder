#pragma once
#ifndef _CAMERADRIVER_H_
#define _CAMERADRIVER_H_

#include <string>
#include <vector>
#include <frame_types.h>
#include <thread>
#include <mutex>
#include <format>
#include <unordered_map>
#include <set>
#include <type_traits>
#define cam_frame_buf_len 16
namespace capture {
    class CameraStream;
    class CameraDevice;
    class CameraProfile {
    public:
        CameraStream* stream;
        CameraProfile(CameraStream* stream) :stream(stream) {}
        Resolution resolution;
        Ratio ratio;
        PIX_TYPE format;
        inline RawFrame* createFrame(long long ts,
            unsigned char* frame, unsigned int len,
            const std::function<void()>& declloc) {
            auto f = new RawFrame{
                frame,len,ts,this
            };
            f->free_funcs.push(declloc);
            f->ref_cnt = 1;
            return f;
        }
        bool is_valid() { return format != PIX_TYPE_ERR; }
        std::string get_profile_str() {
            return std::format("{}x{}@{:.2f}", resolution.width, resolution.height, ratio.get());
        }
        std::string get_profile_codec() {
            return GET_PIX_TYPE_NAME(format);
        }
        virtual ~CameraProfile() {}
        static bool cmp_profile(CameraProfile* a, CameraProfile* b);
        bool operator==(const CameraProfile& a) const {
            return (a.resolution.width == resolution.width &&
                a.resolution.height == resolution.height &&
                a.ratio.denominator == ratio.denominator &&
                a.ratio.numerator == ratio.numerator &&
                a.format == format);
        }
    };

    typedef enum {
        OPTION_INVALID=0,
        OPTION_AUTO,
        OPTION_MANUAL
    } OPTION_TYPE;
    struct option_status {
        int value;
        OPTION_TYPE status_type;
    };
    struct option_range {
        bool is_supported;
        int min;
        int max;
        int step;
        int scaled_factor;
        OPTION_TYPE support_type;
        option_status def;
        option_status current;
    };
    class CameraDevice {
    public:
        std::mutex device_lock;
        std::string device_name;
        std::unordered_map<std::string, CameraStream*> streams_map;
        std::set<CameraStream*> enabled_streams;
        std::condition_variable device_cond;

        typedef enum {
            CS_STANDBY = -1,
            CS_UNINIT = -2,
            CS_ON_STOP = -3,
            CS_RUNNING = 11,
            CS_IGNORE = 0
        } CURRENT_STATUS;
        CURRENT_STATUS status = CS_UNINIT;
        bool start();
        virtual bool native_start() = 0;

        void stop(bool block = true);
        virtual void native_stop() = 0;

        bool init();
        virtual bool native_init()=0;
        bool is_running() {
            std::unique_lock l(device_lock);
            return status == CS_RUNNING || status == CS_ON_STOP;
        }
        void release();
        virtual void native_release()=0;

        void clear();
        bool register_stream(CameraProfile* profile);
        void unregister_stream(CameraStream* stream = NULL);
        void onDeviceReadingFailed() {
            setStatusIf(CameraDevice::CS_STANDBY);
            std::unique_lock l(device_lock);
            clear();
        }

        bool setStatusIf(CURRENT_STATUS set, CURRENT_STATUS cmp = CS_IGNORE) {
            std::unique_lock l(device_lock);
            if (cmp == CS_IGNORE || status == cmp) {
                if (set != CS_IGNORE) {
                    status = set;
                    device_cond.notify_all();
                }
                return true;
            }
            else
                return false;
        }


        typedef enum {
            DEVICE_PAN = 0,
            DEVICE_TILT,
            DEVICE_ROLL,
            DEVICE_ZOOM,
            DEVICE_EXPOSURE,
            DEVICE_IRIS,
            DEVICE_FOCUS,
            DEVICE_LIGHT,
            DEVICE_CONTRAST,
            DEVICE_HUE,
            DEVICE_SATURATION,
            DEVICE_SHARPNESS,
            DEVICE_GAMMA,
            DEVICE_WHITE_BALANCE,
            DEVICE_GAIN,
            DEVICE_BRIGHTNESS,
            DEVICE_BACKLIGHT,
            DEVICE_COLOR_ENABLED,
            DEVICE_OPTION_CNT
        } DEVICE_OPTION;
        option_range configurations[DEVICE_OPTION_CNT] = {0,};


        option_range get_option_range(DEVICE_OPTION option){
            return configurations[option];
        }
        option_status get_option(DEVICE_OPTION option) {
            if (configurations[option].is_supported) {
            return configurations[option].current;
            }
            return {0,};
        }
        bool set_option(DEVICE_OPTION option, const option_status& value) {
            if (configurations[option].is_supported) {
                set_option_native(option, value);
                return true;
            }
            return false;
        }
        option_status get_reset_option(DEVICE_OPTION option) {
            return configurations[option].def;
        }


        //virtual option_status get_option_native(DEVICE_OPTION option) = 0;
        virtual void set_option_native(DEVICE_OPTION option, const option_status& value) = 0;



        std::string device_friendly_name;
        PIX_TYPE encoder_method=PIX_TYPE_MJPG;
        int encoder_quality = 90;
    };
    class CameraStream {
    public:
        using Cmp = std::integral_constant<decltype(&CameraProfile::cmp_profile), &CameraProfile::cmp_profile>;

        std::string stream_name;
        CameraDevice* device;
        CameraProfile* selected_profile = NULL;
        CameraProfile* default_profile = NULL;
        long long last_valid_ts = 0;

        bool is_valid() { return default_profile != NULL; }
        std::unordered_map<std::string, std::set<CameraProfile*,
            Cmp>> profiles_map;

        CameraStream(CameraDevice* device) :device(device) {}
        virtual ~CameraStream() {
            for (auto& [_, ps] : profiles_map)
                for (auto p : ps)
                    delete p;
        }
        void write(RawFrame* item) {
            std::lock_guard<std::mutex> lock(device->device_lock);
            if (frame_queue.size() == capacity_) {
                frame_queue.front()->release();
                frame_queue.pop_front();  // Discard the oldest element
            }else{
                count++;
            }
            frame_queue.push_back(item);
            last_valid_ts = item->frame_ts;
            device->device_cond.notify_all();  // Notify consumer if it's waiting
        }
        void clear() {
            for (auto f : frame_queue) {
                f->release();
            }
            frame_queue.clear();
            last_valid_ts = 0; 

            count = 0;
            previous_fps = 0;
            previous_fps_time = std::chrono::steady_clock::now();
        }
        int wait_for_valid(bool block, std::chrono::steady_clock::time_point tp = {});


        std::deque<RawFrame*> frame_queue;
    public:

        int count = 0;
        float previous_fps = 0;
        std::chrono::steady_clock::time_point previous_fps_time = std::chrono::steady_clock::now();

    private:
        const size_t capacity_=10;
    };

    class CameraDeviceEnumerator { // for devices enum
    public:
        std::vector<CameraDevice*> devices;
        virtual ~CameraDeviceEnumerator() {
            for (auto d : devices)delete d;
        };
        /* Other members */
    };
};

class CameraControl {
public:


};
#endif