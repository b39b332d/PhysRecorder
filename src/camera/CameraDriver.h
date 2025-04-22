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
#include <algorithm>
#include <ImageDecoder.h>
#define cam_frame_buf_len 16
#include <Option.hpp>


namespace capture {

    class CameraStream;
    class CameraDevice;
    class CameraProfile {
    public:
        CameraStream* stream;
        CameraProfile(CameraStream* stream) :stream(stream) {}
        Resolution resolution;
        PIX_TYPE format;
        Ratio ratio;
        inline RawFrame* createFrame(long long ts,
            unsigned char* frame, unsigned int len,
            const std::function<void()>& declloc) {
            auto f = new RawFrame{
                frame,len,resolution,format,ts,this
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

    class CameraDevice :public Options {
    public:
        std::mutex device_lock;
        std::string device_name; // display name
        std::unordered_map<std::string, CameraStream*> streams_map;
        std::set<CameraStream*> enabled_streams;
        std::condition_variable device_cond;

        CameraDevice() :Options(DEVICE_OPTION_CNT) {};
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
        bool is_stream_enabled(CameraStream* stream);
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



    };

#define STREAM_OPTION_LOSSLESS 0 
#define STREAM_OPTION_LOSSY 1
    typedef enum {
        STREAM_CONTRAST=0,
        STREAM_BRIGHTNESS,
        STREAM_SHARPNESS,
        STREAM_WHITEBALANCE,
        STREAM_SATURATION,
        STREAM_VALUE,
        // lossy above
        STREAM_MODE,
        STREAM_FLIP_LR,
        STREAM_FLIP_UD,
        STREAM_ROTATE,
        STREAM_CROP_X,
        STREAM_CROP_Y,
        STREAM_CROP_WIDTH,
        STREAM_CROP_HEIGHT,
        STREAM_OPTION_CNT,
        STREAM_LOSSY_OPTION_CNT = STREAM_MODE
    } STREAM_OPTION;
    class CameraStream: public Options {
        CameraProfile* selected_profile = NULL;
    public:
        inline RawFrame* createEmptyFrame() {
            auto f = new RawFrame{
                nullptr,0,resolution,PIX_TYPE_UNK,0,selected_profile
            };
            return f;
        }
        std::mutex stream_lock;
        Resolution resolution;
        PIX_TYPE format;
        Ratio ratio;
		CameraProfile* get_current_profile() {
			return selected_profile;
		}
        using Cmp = std::integral_constant<decltype(&CameraProfile::cmp_profile), &CameraProfile::cmp_profile>;

        std::string stream_name;
        CameraDevice* device;
        CameraProfile* default_profile = NULL;
        //Transform transform;
        long long last_valid_ts = 0;

        bool is_valid() { return default_profile != NULL; }
        std::unordered_map<std::string, std::set<CameraProfile*,
            Cmp>> profiles_map;

        void set_current_profile(CameraProfile* profile);
        CameraStream(const std::string& stream_name, CameraDevice* device);
        virtual ~CameraStream() {
            for (auto& [_, ps] : profiles_map)
                for (auto p : ps)
                    delete p;
        }
        void write(RawFrame* item) {
            stream_lock.lock();
            postprocess(item, this);
            resolution = item->resolution;
			format = item->format;
            stream_lock.unlock();
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

        PIX_TYPE encoder_method = PIX_TYPE_MJPG;
        int encoder_quality = 90;
        std::string stream_friendly_name; // save name
    private:
        const size_t capacity_=50; // max 1000 fps
    };

};

#endif