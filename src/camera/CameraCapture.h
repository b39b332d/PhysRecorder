#ifndef _CAMERACAPTURE_H_
#define _CAMERACAPTURE_H_
#include "CameraDriver.h"
#include <unordered_map>
#include <unordered_set>



namespace capture{
	typedef  std::unordered_map<CameraStream*, std::vector<RawFrame*>> frame_set_t;
	typedef  std::unordered_map<std::string, CameraDevice*> devices_bimap_t;
	typedef  std::unordered_set<CameraDevice*> devices_set_t;
	DRIVER_API
		void refresh_devices(devices_set_t& devices_map);
	DRIVER_API
		void readFrames(frame_set_t& frames,
		std::chrono::steady_clock::time_point until,
		devices_set_t& new_disabled_devices,
		devices_set_t& all_devices,
		CameraStream* wait_stream = nullptr );
	DRIVER_API
		bool enable_device(CameraDevice*);
	DRIVER_API
		void disable_device(CameraDevice*);
};


#endif