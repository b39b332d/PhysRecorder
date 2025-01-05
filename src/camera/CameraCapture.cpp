#include "CameraCapture.h"
#include "DriverEnum.h"
#include <thread>
namespace capture {

	devices_set_t enabled_devices;
	std::mutex devices_lock;

	/*
		bidirectional map
		current_devices->first <--> current_devices->second->device_name
	*/
	static devices_bimap_t current_devices;
	void refresh_devices(devices_set_t& devices_map)
	{
		static bool first_run = true;
		if (first_run) {
			first_run = false;
		}
		std::unique_lock l(devices_lock);
		for (auto it = current_devices.cbegin(); it != current_devices.cend() /* not hoisted */; /* no increment */)
		{
			if (it->second->status == CameraDevice::CS_STANDBY || it->second->status == CameraDevice::CS_UNINIT) {
				delete it->second;
				current_devices.erase(it++);
			}
			else {
				devices_map.insert(it->second);
				++it;
			}
		}
		std::vector<std::vector<CameraDevice*>> dss;
		for (int i = 0; i < sizeof(enum_drivers)/sizeof(enum_drivers[0]); i++) {
			dss.push_back(enum_drivers[i]());
		}
		for (auto& ds : dss) {
			for (auto d : ds) {
				auto pd = current_devices.find(d->device_name);
				if (pd != current_devices.end()) {
					delete d;
				}
				else {
					current_devices[d->device_name] = d;
					devices_map.insert(d);
				}
			}
		}
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
		devices_set_t& new_disabled_devices,
		devices_set_t& enabled_devices_temp,
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