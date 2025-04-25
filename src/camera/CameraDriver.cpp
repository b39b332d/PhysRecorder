#include <CameraDriver.h>
#include <algorithm>
#include <chrono>
namespace capture {



	void CameraDevice::clear() {
		for (auto s : enabled_streams) {
			s->clear();
		}
	}
	bool CameraDevice::register_stream(CameraProfile* profile) {
        std::unique_lock dl(device_lock);
		enabled_streams.insert(profile->stream);
		profile->stream->set_current_profile(profile);
		return true;
	}
    bool CameraDevice::is_stream_enabled(CameraStream* stream) {
        std::unique_lock dl(device_lock);
        return enabled_streams.contains(stream);
    }

    bool CameraDevice::init() {
        if (status == CS_STANDBY) {
            unregister_stream(); //clear all info!!!
            return true;
        }
        else if (status != CS_UNINIT)
            return false;
        if (native_init()) {
            status = CS_STANDBY;
            for (auto [_, s] : streams_map) {
                s->set_current_profile(s->default_profile);
            }
            return true;
        }
        else
            return false;
    };
	void CameraDevice::stop(bool block) {
        if (status == CameraDevice::CS_RUNNING) {
            std::unique_lock dl(device_lock);
            status = CS_ON_STOP;
            device_cond.notify_all();
            dl.unlock();

            native_stop();
            if (block) {
                dl.lock();
                device_cond.wait(dl, [this]() {
                    return status == CameraDevice::CS_STANDBY;
                    });
                clear();
            }
            return;
        }
        else if (status == CameraDevice::CS_ON_STOP) {
            std::unique_lock dl(device_lock);
            if (block) {
                device_cond.wait(dl, [this]() {
                    return status == CameraDevice::CS_STANDBY;
                    });
                clear();
            }
            else {
                clear();
            }
        }
    }
    bool CameraDevice::start() {
        std::unique_lock l(device_lock);
        if (enabled_streams.size() == 0)
            return false;
        for (auto& s : enabled_streams) {
                s->last_valid_ts = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        }
        if (native_start()) {
            status = CameraDevice::CS_RUNNING;
            return true;
        }
        return false;
    }

    CameraStream::CameraStream(const std::string& stream_name, CameraDevice* device) :
        device(device), stream_name(stream_name), Options(STREAM_OPTION_CNT) {
        stream_friendly_name = (device ?device->device_name:"Unknow") + "_" + stream_name;
        std::replace(stream_friendly_name.begin(), stream_friendly_name.end(), ':', '-');

        configurations[STREAM_MODE] = { true, STREAM_OPTION_LOSSLESS,STREAM_OPTION_LOSSY, 1, 1, OPTION_AUTO,{ STREAM_OPTION_LOSSLESS,OPTION_MANUAL },{ STREAM_OPTION_LOSSLESS,OPTION_MANUAL } };
        configurations[STREAM_FLIP_LR] = { true, 0,1, 1, 1, OPTION_MANUAL,{ 0,OPTION_MANUAL },{ 0,OPTION_MANUAL } };
        configurations[STREAM_FLIP_UD] = { true, 0,1, 1, 1, OPTION_MANUAL,{ 0,OPTION_MANUAL },{ 0,OPTION_MANUAL } };
        configurations[STREAM_ROTATE] = { true, 0, 360, 1, 1, OPTION_MANUAL,{ 0,OPTION_MANUAL },{ 0,OPTION_MANUAL } };
        configurations[STREAM_CONTRAST] = { true, 50, 300, 1, 100, OPTION_MANUAL,{ 100,OPTION_MANUAL },{ 100,OPTION_MANUAL } };
        configurations[STREAM_BRIGHTNESS] = { true, 0, 100, 1, 1, OPTION_MANUAL,{ 0,OPTION_MANUAL },{ 0,OPTION_MANUAL } };
        configurations[STREAM_SHARPNESS] = { true, -30, 30, 1, 10, OPTION_MANUAL,{ 0,OPTION_MANUAL },{ 0,OPTION_MANUAL } };
        configurations[STREAM_SATURATION] = { true, 50, 300, 1, 100, OPTION_MANUAL,{ 100,OPTION_MANUAL },{ 100,OPTION_MANUAL } };
        configurations[STREAM_VALUE] = { true, -30, 30, 1, 1, OPTION_AUTO,{ 0,OPTION_MANUAL },{ 0,OPTION_MANUAL } };
        configurations[STREAM_GAMMA] = { true, 0, 300, 1, 100, OPTION_AUTO,{ 100,OPTION_MANUAL },{ 100,OPTION_MANUAL } };
    }
    void CameraStream::set_current_profile(CameraProfile* profile)
    {
        //if (orig pix >= 24) {
        //    get_option(STREAM_CONTRAST].is_supported = true;
        //    get_option(STREAM_BRIGHTNESS].is_supported = true;
        //    get_option(STREAM_SHARPNESS].is_supported = true;
        //    get_option(STREAM_WHITEBALANCE].is_supported = true;
        //    get_option(STREAM_SATURATION].is_supported = true;
        //    get_option(STREAM_VALUE].is_supported = true;
        //}
        //else {
        //    // opencv one channel or mjpeg
        //    get_option(STREAM_CONTRAST].is_supported = false;
        //    get_option(STREAM_BRIGHTNESS].is_supported = false;
        //    get_option(STREAM_SHARPNESS].is_supported = false;
        //    get_option(STREAM_WHITEBALANCE].is_supported = false;
        //    get_option(STREAM_SATURATION].is_supported = false;
        //    get_option(STREAM_VALUE].is_supported = false;
        //}
        int width = (int)(profile->resolution.width);
        int height = (int)(profile->resolution.height);
        int x = 0, y = 0, width_dst = width, height_dst = height;
        if (selected_profile != nullptr) {
			x = get_option_value(STREAM_CROP_X) * width / selected_profile->resolution.width;
            y = get_option_value(STREAM_CROP_Y) * height / selected_profile->resolution.height ;
            width_dst = get_option_value(STREAM_CROP_WIDTH) * width / selected_profile->resolution.width ;
            height_dst = get_option_value(STREAM_CROP_HEIGHT) * height / selected_profile->resolution.height ;
        }
        set_option_range(STREAM_CROP_X, { true,0,width/2,1,0.5,OPTION_MANUAL,{0,OPTION_MANUAL},{x/2,OPTION_MANUAL} });
        set_option_range(STREAM_CROP_Y, { true,0,height/2,1,0.5,OPTION_MANUAL,{0,OPTION_MANUAL},{y/2,OPTION_MANUAL} });
        set_option_range(STREAM_CROP_WIDTH, { true,0,width/2,1,0.5,OPTION_MANUAL,{width/2,OPTION_MANUAL},{width_dst/2,OPTION_MANUAL} });
        set_option_range(STREAM_CROP_HEIGHT, { true,0,height/2,1,0.5,OPTION_MANUAL,{height/2,OPTION_MANUAL},{height_dst/2,OPTION_MANUAL} });

        selected_profile = profile;
		ratio = profile->ratio;
		resolution = profile->resolution;
		format = profile->format;
        return;
    }
    int CameraStream::wait_for_valid(bool block, std::chrono::steady_clock::time_point tp) {
        long long current_ts = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

        std::unique_lock<std::mutex> lock(device->device_lock);
        if (device->status != CameraDevice::CS_RUNNING)
            return -1;
        if (last_valid_ts != 0 && current_ts - last_valid_ts > 5e6) {
            lock.unlock();
            auto t = new std::thread([this]() {device->stop(false); });
            t->detach();
            delete t;
            return -1;
        }
        if (!block) {
            if (frame_queue.empty()) {
                return 0;
            }
            return frame_queue.size();
        }
        else {
            device->device_cond.wait_until(lock,
                tp,
                [this]() { return !frame_queue.empty() ||
                device->status != CameraDevice::CS_RUNNING; });
            if (device->status != CameraDevice::CS_RUNNING) return -1;
            else return frame_queue.size();
        }
    }
    bool CameraProfile::cmp_profile(CameraProfile* a, CameraProfile* b){
        int size_a = a->resolution.width * a->resolution.height;
        int size_b = b->resolution.width * b->resolution.height;
        if (size_a > size_b)
            return true;
        else if (size_a == size_b) {
            if (a->ratio.get() > b->ratio.get()) {
                return true;
            }
            else if (a->ratio.get() == b->ratio.get()) {
                if (a->resolution.width > b->resolution.width)
                    return true;
                else if (a->resolution.width == b->resolution.width)
                    if (a->ratio.numerator > a->ratio.numerator)
                        return true;
            }
        }
        return false;
    }


    void CameraDevice::release(){
        stop();
        for (auto& [_, s] : streams_map) {
            s->clear();
            delete s;
        }
        streams_map.clear();
        native_release();
        status = CS_UNINIT;
    };

    void CameraDevice::unregister_stream(CameraStream* stream) {
        std::unique_lock l(device_lock);
        if (status == CameraDevice::CS_STANDBY) {
            if (stream == NULL) {
                enabled_streams.clear();
                clear();
            }
            else {
                enabled_streams.erase(stream);
                stream->set_current_profile(stream->default_profile);
            }
        }
    }
};