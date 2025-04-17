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
        profile->stream->selected_profile = profile;
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
                s->selected_profile = s->default_profile;
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
            if (block) {
                std::unique_lock dl(device_lock);
                device_cond.wait(dl, [this]() {
                    return status == CameraDevice::CS_STANDBY;
                    });
                clear();
            }
            else {
                std::unique_lock dl(device_lock); 
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
                stream->selected_profile = stream->default_profile;
            }
        }
    }
};