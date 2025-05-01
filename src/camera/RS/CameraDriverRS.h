#ifndef _CAMERADRIVER_RS_H_
#define _CAMERADRIVER_RS_H_

#include <string>
#include <CameraDriver.h>
#include <librealsense2/rs.hpp>
namespace capture {
    class CameraStreamRS;
    class CameraProfileRS :public CameraProfile {
    public:

        rs2::stream_profile rs_profile;
        std::string stream_name;
        CameraProfileRS(rs2::stream_profile& rs_profile, CameraStream* stream);
        ~CameraProfileRS();
    };

    class CameraDeviceRS;
    class CameraStreamRS :public CameraStream {
    public:
        int stream_index;
        rs2::sensor rs_sensor;
        CameraStreamRS(const std::string& stream_name,rs2::sensor&, CameraDevice* device);

    };
    class CameraDeviceRS :public CameraDevice {
        rs2::device rs_device;
        rs2::sensor rs_sensor;
        bool is_color;
        bool is_led;
    public:
        CameraDeviceRS(rs2::device& rs_device, rs2::sensor& rs_sensor);
        bool native_init();
        bool native_start();
        void native_stop();
        void native_release();

         void get_all_option_range_native();
         void set_option_native(int option, const option_status& value) override;

    private:
        void set_signle_option_native(int option, rs2_option opt, const option_status& value);
    };

    std::vector<CameraDevice*> EnumerateCamera_RS();
}
#endif