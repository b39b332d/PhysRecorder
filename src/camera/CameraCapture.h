#ifndef _CAMERACAPTURE_H_
#define _CAMERACAPTURE_H_
#include "CameraDriver.h"
#include <map>


namespace capture{
	typedef  std::map<CameraStream*, std::vector<RawFrame*>> frame_set_t;
	void refresh_devices();
	void readFrames(frame_set_t& frames, 
		std::chrono::steady_clock::time_point until,
		std::set<CameraDevice*>& new_disabled_devices,
		std::set<CameraDevice*>& all_devices,
		CameraStream* wait_stream = nullptr );
	bool enable_device(CameraDevice*);
	void disable_device(CameraDevice*);
	extern std::map<std::string, CameraDevice*> devices_map;
};


#endif