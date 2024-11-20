#include "CameraCapture.h"

#include "MSMF/CameraDriverMSMF.h"
#include "RS/CameraDriverRS.h"
#include <thread>
namespace capture {
	std::map<std::string, CameraDevice*> devices_map;
	std::set<CameraDevice*> enabled_devices;
	std::mutex devices_lock;
	void refresh_devices()
	{
		std::unique_lock l(devices_lock);
		for (auto it = devices_map.cbegin(); it != devices_map.cend() /* not hoisted */; /* no increment */)
		{
			if (it->second->status == CameraDevice::CS_STANDBY || it->second->status == CameraDevice::CS_UNINIT) {
				delete it->second;
				devices_map.erase(it++);
			}
			else
				++it;
		}
		std::map<std::string, CameraDevice*> devices_new_map;
		std::vector<std::vector<CameraDevice*>> dss;
		dss.push_back(EnumerateCamera_MSMF());
		dss.push_back(EnumerateCamera_RS());
		for (auto& ds : dss) {
			for (auto d : ds) {
				auto pd = devices_map.find(d->device_name);
				if (pd != devices_map.end()) {
					delete d;
				}
				else {
					devices_new_map[d->device_name] = d;
				}
			}
		}
		devices_map = devices_new_map;
	}
	bool enable_device(CameraDevice* device) {
		if (device->start()) {
			std::unique_lock l(devices_lock);
			enabled_devices.insert(device);
		}
		else
			return false;
		return true;
	}
	void disable_device(CameraDevice* device) {
		std::unique_lock l(devices_lock);
		if (enabled_devices.find(device) != enabled_devices.end()) {
			enabled_devices.erase(device);
			l.unlock();
			device->stop(true);
		}
		return;
	}



	void readFrames(
		frame_set_t& frames, 
		std::chrono::steady_clock::time_point until, 
		std::set<CameraDevice*>& new_disabled_devices,
		std::set<CameraDevice*>& enabled_devices_temp,
		CameraStream* wait_stream
		)
	{

		bool has_frame;
		devices_lock.lock();
		enabled_devices_temp = enabled_devices;
		devices_lock.unlock();


		if (wait_stream) {
			if (enabled_devices_temp.find(wait_stream->device) == enabled_devices_temp.end())
				return;
			else {
				int buf_len = wait_stream->wait_for_valid(true, until);
				if (buf_len < 0) {
					wait_stream->device->device_lock.lock();
					wait_stream->device->clear();
					wait_stream->device->device_lock.unlock();
					enabled_devices_temp.erase(wait_stream->device);
					new_disabled_devices.insert(wait_stream->device);
				}
				else if (buf_len > 0) {
					has_frame = true;
					goto end_lp;
				}
			}
		}
		else {
			std::this_thread::sleep_until(until);
		}
		if (enabled_devices_temp.size() == 0)
			return ;
	end_lp:
		for (auto it = enabled_devices_temp.begin(); it != enabled_devices_temp.end(); ) {
			auto d = *it;
			it++;
			for (auto s : d->enabled_streams) {
				int status = s->wait_for_valid(false, {});
				if (status == -1) {
					d->clear();
					enabled_devices_temp.erase(d);
					new_disabled_devices.insert(d);
					break;
				}
				else if (status > 0) {
					std::unique_lock l(d->device_lock);
					while (s->frame_queue.size() != 0) {
						auto frame = s->frame_queue.front();
						s->frame_queue.pop_front();
						frames[s].push_back(frame);
					}
				}
			}
		}

		devices_lock.lock();
		for (auto d : new_disabled_devices) {
			enabled_devices.erase(d);
		}
		devices_lock.unlock();
	}

}