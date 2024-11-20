#include <CameraDriver.h>
#include <iostream>
#include <opencv2/imgproc.hpp>
#include "CameraCapture.h"
int main() {

	capture::refresh_devices();
	auto d1 = capture::devices_map[std::string("MF: HIK 2K Camera @vid_2bdf")];
	auto d2 = capture::devices_map[std::string("RS: Intel RealSense D455 RGB Camera: #215122254399")];
	d1->init();
	d2->init();
	d1->streams_map["Video:0"]->selected_profile = d1->streams_map["Video:0"]->default_profile;
	d2->streams_map["Color"]->selected_profile = d2->streams_map["Color"]->default_profile;
	capture::enable_device(d1);
	capture::enable_device(d2);
	std::cout << d1->streams_map["Video:0"]->selected_profile->get_profile_str() << std::endl;
	std::cout << d2->streams_map["Color"]->selected_profile->get_profile_str() << std::endl;
	for (int i = 0; i < 100; i++) {
		capture::frame_set_t frames;
		capture::readFrames(frames, std::chrono::steady_clock::now()+ std::chrono::milliseconds(100));
		for (auto &[s_N,ff] : frames) {
			std::cout << s_N->device->device_name << ":\t" << ff.size() << std::endl;;

			for (auto f : ff) {
				delete f;
			}
		}
		std::cout << "---------" << std::endl;
	}	
	capture::disable_device(d1);
	capture::disable_device(d2);

	for(auto &[_,d]: capture::devices_map){
		std::cout << std::endl;
		std::cout << d->device_name << std::endl;
		d->init();
		for (auto &[s_n,s] : d->streams_map) {
			std::cout << "    " << s_n << " -def- " << s->default_profile->get_profile_str() << " " <<
				s->default_profile->get_profile_codec() << std::endl;
			for (auto& [stream_type, profiles] : s->profiles_map) {
				std::cout << "        " << stream_type << std::endl;
				for (auto p : profiles) {
					std::cout << "            " << p->get_profile_str() << "  ";

					s->selected_profile = p;
					capture::enable_device(d);
					for (int i = 0; i < 50; i++) {
						capture::frame_set_t frames;
						capture::readFrames(frames, std::chrono::steady_clock::now()+ std::chrono::milliseconds(100));
						for (auto& [s_N, ff] : frames) {
							std::cout << s_N->device->device_name << ":\t" << ff.size() << std::endl;;

							for (auto f : ff) {
								delete f;
							}
						}
					}
					capture::disable_device(d);

					std::cout << std::endl;
				}
			}
		}
	}
}