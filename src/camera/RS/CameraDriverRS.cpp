#include "CameraDriverRS.h"

#include <codecvt>
#include <iostream>

namespace capture {


#ifndef CAMRS_IF_EQUAL_RETURN
#define CAMRS_IF_EQUAL_RETURN(val) if(RS2_FORMAT_##val == prof) return PIX_TYPE_##val
#endif
    inline PIX_TYPE RS_Profile_to_pixformat(rs2_format prof) {
        CAMRS_IF_EQUAL_RETURN(YUYV);
        CAMRS_IF_EQUAL_RETURN(UYVY);
        CAMRS_IF_EQUAL_RETURN(RGB8);
        CAMRS_IF_EQUAL_RETURN(BGR8);
        if (RS2_FORMAT_RGBA8 == prof) return PIX_TYPE_RGBA;
        if (RS2_FORMAT_BGRA8 == prof) return PIX_TYPE_BGRA;
        if (RS2_FORMAT_Z16 == prof) return PIX_TYPE_D16;
        if (RS2_FORMAT_Y8 == prof) return PIX_TYPE_L8;
        if (RS2_FORMAT_Y16 == prof) return PIX_TYPE_L16;
        if (RS2_FORMAT_RAW10 == prof) return PIX_TYPE_RS10;
        if (RS2_FORMAT_RAW8 == prof) return PIX_TYPE_RS8;
        if (RS2_FORMAT_RAW16 == prof) return PIX_TYPE_RS16;
        if (RS2_FORMAT_Y16 == prof) return PIX_TYPE_L16;
        if (RS2_FORMAT_MJPEG == prof) return PIX_TYPE_MJPG;
        std::cout << "Unknow format: " << rs2_format_to_string(prof) << std::endl;
        return PIX_TYPE_ERR;
    }
    CameraDeviceRS::CameraDeviceRS(rs2::device& rs_device, rs2::sensor& rs_sensor) :
        rs_device(rs_device), rs_sensor(rs_sensor)
    {
        auto converter = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>();
        device_name = "Unknown Device";
        if (rs_device.supports(RS2_CAMERA_INFO_NAME))
            device_name = rs_device.get_info(RS2_CAMERA_INFO_NAME);

        // and the serial number of the device:
        std::string sn = "#######";
        if (rs_device.supports(RS2_CAMERA_INFO_SERIAL_NUMBER))
            sn = rs_device.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);
        std::string sensor_name = std::string(rs_sensor.get_info(RS2_CAMERA_INFO_NAME)) + ":";

        device_name = "RS: " + device_name + " " + sensor_name + " #" + sn;
    }

    bool CameraDeviceRS::native_init()
    {
        std::string sensor_name = std::string(rs_sensor.get_info(RS2_CAMERA_INFO_NAME)) + ":";
        std::vector<rs2::stream_profile> stream_profiles = rs_sensor.get_stream_profiles();
        for (rs2::stream_profile stream_profile : stream_profiles)
        {
            std::string stream_name = stream_profile.stream_name();
            CameraStream*& cam_stream = streams_map[stream_name];
            CameraProfileRS* profile = NULL;
            if (cam_stream == NULL) {
                cam_stream = new CameraStreamRS(stream_name, rs_sensor, this);
                profile = new CameraProfileRS(stream_profile, cam_stream);
                cam_stream->default_profile = profile;
                ((CameraStreamRS*)(cam_stream))->stream_index = profile->rs_profile.stream_index();
            }
            else {
                profile = new CameraProfileRS(stream_profile, cam_stream);
            }

            if (profile->is_valid()) {
                cam_stream->profiles_map[GET_PIX_TYPE_NAME(profile->format)].insert
                (profile);
                if (stream_profile.is_default()) {
                    cam_stream->default_profile = profile;
                }
            }
            else {
                delete profile;
            }

        }
        get_all_option_range_native();
        return true;
    }

    bool CameraDeviceRS::native_start()
    {
        std::vector<rs2::stream_profile> pfs;
        for (auto& s : enabled_streams) {
            pfs.push_back(((CameraProfileRS*)(s->selected_profile))->rs_profile);
        }
        if (enabled_streams.size() == 0)
            return false;
        try {
            rs_sensor.open(pfs);
            long long* since = new long long(LLONG_MAX);
            bool first_run = true;
            std::shared_ptr<CameraDeviceRS> t(this, [since](CameraDeviceRS* device) {
                device->onDeviceReadingFailed(); if (since) { delete since; } });

            rs_sensor.start([this, t, since](rs2::frame frame) {
                CameraStream* pts = nullptr;
                long long current_ts = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

                device_lock.lock();
                if (status != CameraDevice::CS_RUNNING) {
                    device_lock.unlock();
                    return;
                }
                for (auto s : enabled_streams) {
                    if (((CameraStreamRS*)s)->stream_index == frame.get_profile().stream_index()) {
                        pts = s;
                    }
                }
                device_lock.unlock();
                if (pts == nullptr) {
                    stop(false);
                    return;
                }
                if (*since == LLONG_MAX) *since = current_ts- frame.get_timestamp() * 1e3;
                pts->write(pts->selected_profile->createFrame(
                     frame.get_timestamp() * 1e3+ *since, (unsigned char*)(frame.get_data()), frame.get_data_size(), [frame]() {

                    }
                ));
            });
        }
        catch (...) {
            return false;
        }
        return true;
    }


    void CameraDeviceRS::native_stop() {
        rs_sensor.stop();
        rs_sensor.close();
    }
    void CameraDeviceRS::native_release() {

    }
    double get_scale_factor(float val) {
        val = fabs(val);
        double i = 1;
        while (val-int(val) > 1e-6) {
            val *= 10;
            i *= 10;
        }
        return i;
    }
    inline option_range get_option_sensor(rs2::sensor& rs_sensor,rs2_option option) {
        option_range opt_range = { 0, };
        if (rs_sensor.supports(option)) {
            rs2::option_range range = rs_sensor.get_option_range(option);
            opt_range.scaled_factor = get_scale_factor(range.step);
            opt_range.min = roundf(range.min * opt_range.scaled_factor);
            opt_range.max = roundf(range.max *  opt_range.scaled_factor);
            opt_range.is_supported = true;
            opt_range.step = roundf(range.step * opt_range.scaled_factor);
            opt_range.def.value = roundf(range.def *opt_range.scaled_factor);
            opt_range.def.status_type = OPTION_MANUAL;
            opt_range.support_type = OPTION_MANUAL;
            opt_range.current.value = roundf(rs_sensor.get_option(option) * opt_range.scaled_factor);
            opt_range.current.status_type = OPTION_MANUAL;
        }
        return opt_range;
    }
    void CameraDeviceRS::get_all_option_range_native() {

        for (int i = 0; i < DEVICE_OPTION_CNT; i++) {
            switch ((DEVICE_OPTION)i) {
            case CameraDevice::DEVICE_EXPOSURE:
                if (rs_sensor.supports(RS2_OPTION_EXPOSURE)) {
                    option_range opt_range = { 0, };
                    rs2::option_range range = rs_sensor.get_option_range(RS2_OPTION_EXPOSURE);
                    opt_range.scaled_factor = get_scale_factor(range.step);
                    opt_range.min = roundf(range.min * opt_range.scaled_factor);
                    opt_range.max = roundf(range.max * opt_range.scaled_factor);;
                    opt_range.is_supported = true;
                    opt_range.step = roundf(range.step * opt_range.scaled_factor);;
                    opt_range.def.value = roundf(range.def * opt_range.scaled_factor);;
                    opt_range.def.status_type = OPTION_MANUAL;
                    if (rs_sensor.supports(RS2_OPTION_ENABLE_AUTO_EXPOSURE)) {
                        rs2::option_range range = rs_sensor.get_option_range(RS2_OPTION_ENABLE_AUTO_EXPOSURE);
                        if (roundf(range.def) == 1)
                            opt_range.def.status_type = OPTION_AUTO;
                        opt_range.support_type = OPTION_AUTO;
                        if (roundf(rs_sensor.get_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE)) == 1)
                            opt_range.current.status_type = OPTION_AUTO;
                        else
                            opt_range.current.status_type = OPTION_MANUAL;
                    }
                    else {
                        opt_range.current.status_type = OPTION_MANUAL;
                        opt_range.support_type = OPTION_MANUAL;
                    }
                    opt_range.current.value = roundf(rs_sensor.get_option(RS2_OPTION_EXPOSURE) * opt_range.scaled_factor);
                    configurations[i] = opt_range;
                }
                break;;
            case CameraDevice::DEVICE_WHITE_BALANCE:
                if (rs_sensor.supports(RS2_OPTION_WHITE_BALANCE)) {
                    option_range opt_range = { 0, };
                    rs2::option_range range = rs_sensor.get_option_range(RS2_OPTION_WHITE_BALANCE);
                    opt_range.scaled_factor = get_scale_factor(range.step);
                    opt_range.min = roundf(range.min * opt_range.scaled_factor);
                    opt_range.is_supported = true;
                    opt_range.max = roundf(range.max * opt_range.scaled_factor);;
                    opt_range.step = roundf(range.step * opt_range.scaled_factor);;
                    opt_range.def.value = roundf(range.def * opt_range.scaled_factor);;
                    opt_range.def.status_type = OPTION_MANUAL;
                    if (rs_sensor.supports(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE)) {
                        rs2::option_range range = rs_sensor.get_option_range(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE);
                        if (roundf(range.def) == 1)
                            opt_range.def.status_type = OPTION_AUTO;
                        opt_range.support_type = OPTION_AUTO;
                        if (roundf(rs_sensor.get_option(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE)) == 1)
                            opt_range.current.status_type = OPTION_AUTO;
                        else
                            opt_range.current.status_type = OPTION_MANUAL;
                    }
                    else {
                        opt_range.current.status_type = OPTION_MANUAL;
                        opt_range.support_type = OPTION_MANUAL;
                    }
                    opt_range.current.value = roundf(rs_sensor.get_option(RS2_OPTION_WHITE_BALANCE) * opt_range.scaled_factor);
                    configurations[i] = opt_range;

                }
                break;
            case CameraDevice::DEVICE_GAIN:
                if (rs_sensor.supports(RS2_OPTION_DIGITAL_GAIN)) {
                    is_color = false;
                    option_range opt_range = { 0, };
                    rs2::option_range range = rs_sensor.get_option_range(RS2_OPTION_DIGITAL_GAIN);
                    opt_range.scaled_factor = 0;
                    opt_range.min = 1;
                    opt_range.is_supported = true;
                    opt_range.max = 2;
                    opt_range.step = 1;
                    if (int(range.def) == RS2_DIGITAL_GAIN_AUTO) {
                        opt_range.def.value = RS2_DIGITAL_GAIN_LOW;
                        opt_range.def.status_type = OPTION_AUTO;
                    }
                    else {
                        opt_range.def.value = range.def;
                        opt_range.def.status_type = OPTION_MANUAL;
                    }
                    if (range.min < 0.1)
                        opt_range.support_type = OPTION_AUTO;
                    else
                        opt_range.support_type = OPTION_MANUAL;
                    auto o = rs_sensor.get_option(RS2_OPTION_DIGITAL_GAIN);
                    if (roundf(o) == 0) {
                        opt_range.current.value = 1;
                        opt_range.current.status_type = OPTION_AUTO;
                    }
                    else {
                        opt_range.current.value = roundf(o);
                        opt_range.current.status_type = OPTION_MANUAL;
                    }
                    configurations[i] = opt_range;
                }
                else {
                    auto color_gain = get_option_sensor(rs_sensor, RS2_OPTION_GAIN);
                    is_color = true;
                    configurations[DEVICE_GAIN] = color_gain;
                }
                break;
            case CameraDevice::DEVICE_LIGHT:
                if (rs_sensor.supports(RS2_OPTION_LASER_POWER)) {
                    is_led = false;
                    option_range opt_range = { 0, };
                    rs2::option_range range = rs_sensor.get_option_range(RS2_OPTION_LASER_POWER);
                    opt_range.scaled_factor = get_scale_factor(range.step);
                    opt_range.min = roundf(range.min * opt_range.scaled_factor);
                    opt_range.is_supported = true;
                    opt_range.max = roundf(range.max * opt_range.scaled_factor);;
                    opt_range.step = roundf(range.step * opt_range.scaled_factor);;
                    opt_range.def.value = roundf(range.def * opt_range.scaled_factor);;
                    opt_range.def.status_type = OPTION_MANUAL;
                    opt_range.current.value = roundf(rs_sensor.get_option(RS2_OPTION_LASER_POWER) * opt_range.scaled_factor);
                    opt_range.current.status_type = OPTION_MANUAL;
                    opt_range.support_type = OPTION_MANUAL;
                    if (rs_sensor.supports(RS2_OPTION_EMITTER_ENABLED)) {
                        rs2::option_range range = rs_sensor.get_option_range(RS2_OPTION_EMITTER_ENABLED);
                        if (range.max > 1.9) {
                            opt_range.support_type = OPTION_AUTO;
                            int c = roundf(rs_sensor.get_option(RS2_OPTION_EMITTER_ENABLED));
                            if (c == 2) {
                                opt_range.current.status_type = OPTION_AUTO;
                            }else if(c==0)
                                opt_range.current.value = 0;
                        }
                        if (roundf(range.def) == 2)
                            opt_range.def.status_type = OPTION_AUTO;
                    }
                    configurations[i] = opt_range;
                }
                else if(rs_sensor.supports(RS2_OPTION_LED_POWER)) {
                    is_led = true;
                    option_range opt_range = { 0, };
                    rs2::option_range range = rs_sensor.get_option_range(RS2_OPTION_LED_POWER);
                    opt_range.scaled_factor = get_scale_factor(range.step);
                    opt_range.min = roundf(range.min * opt_range.scaled_factor);
                    opt_range.is_supported = true;
                    opt_range.max = roundf(range.max * opt_range.scaled_factor);;
                    opt_range.step = roundf(range.step * opt_range.scaled_factor);;
                    opt_range.def.value = roundf(range.def * opt_range.scaled_factor);;
                    opt_range.def.status_type = OPTION_MANUAL;
                    opt_range.current.value = roundf(rs_sensor.get_option(RS2_OPTION_LED_POWER) * opt_range.scaled_factor);
                    opt_range.current.status_type = OPTION_MANUAL;
                    opt_range.support_type = OPTION_MANUAL;
                    configurations[i] = opt_range;
                }
                break;
            case CameraDevice::DEVICE_GAMMA:
                configurations[i] = get_option_sensor(rs_sensor, RS2_OPTION_GAMMA);
                break;
            case DEVICE_CONTRAST:
                configurations[i] = get_option_sensor(rs_sensor, RS2_OPTION_CONTRAST);
                break;
            case DEVICE_HUE:
                configurations[i] = get_option_sensor(rs_sensor, RS2_OPTION_HUE);
                break;
            case DEVICE_SATURATION:
                configurations[i] = get_option_sensor(rs_sensor, RS2_OPTION_SATURATION);
                break;
            case DEVICE_SHARPNESS:
                configurations[i] = get_option_sensor(rs_sensor, RS2_OPTION_SHARPNESS);
                break;
            case DEVICE_BACKLIGHT:
                configurations[i] = get_option_sensor(rs_sensor, RS2_OPTION_BACKLIGHT_COMPENSATION);
                break;
            case DEVICE_BRIGHTNESS:
                configurations[i] = get_option_sensor(rs_sensor, RS2_OPTION_BRIGHTNESS);
                break;

            }
        }
    }
    inline void CameraDeviceRS::set_signle_option_native(DEVICE_OPTION option,rs2_option opt, const option_status& value) {
        float value_o = value.value / configurations[option].scaled_factor;
        if (configurations[option].step > 1) {
            if (value.value % configurations[option].step != 0) {
                float s = (float)(value.value - configurations[option].min) / configurations[option].step;
                value_o = (roundf(s) * configurations[option].step + configurations[option].min) / configurations[option].scaled_factor;
            }
        }
        rs_sensor.set_option(opt, value_o);
        configurations[option].current.value = value.value;
    }

    void CameraDeviceRS::set_option_native(DEVICE_OPTION option, const option_status& value)
    {
        switch (option) {
        case DEVICE_EXPOSURE:
            if (value.status_type == OPTION_AUTO) {
                rs_sensor.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, 1);
                configurations[option].current.status_type = OPTION_AUTO;
            }
            else {
                float value_o = value.value / configurations[option].scaled_factor;
                if (configurations[option].step > 1) {
                    if (value.value % configurations[option].step != 0) {
                        float s = (float)(value.value - configurations[option].min) / configurations[option].step;
                        value_o = (roundf(s) * configurations[option].step + configurations[option].min) /  configurations[option].scaled_factor;
                    }
                }
                rs_sensor.set_option(RS2_OPTION_EXPOSURE, value_o);
                configurations[option].current.value = value.value;
            }
            break;
        case CameraDevice::DEVICE_WHITE_BALANCE:
            if (value.status_type == OPTION_AUTO) {
                rs_sensor.set_option(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE, 1);
                configurations[option].current.status_type = OPTION_AUTO;
            }
            else {
                float value_o = value.value / configurations[option].scaled_factor;
                if (configurations[option].step > 1) {
                    if (value.value % configurations[option].step != 0) {
                        float s = (float)(value.value - configurations[option].min) / configurations[option].step;
                        value_o = (roundf(s) * configurations[option].step + configurations[option].min) / configurations[option].scaled_factor;
                    }
                }
                rs_sensor.set_option(RS2_OPTION_WHITE_BALANCE, value_o);
                configurations[option].current.value = value.value;
            }
            break;
        case CameraDevice::DEVICE_GAIN:
            if (is_color) {
                set_signle_option_native(DEVICE_GAIN, RS2_OPTION_GAIN, value);
            }
            else {
                if (value.status_type == OPTION_AUTO) {
                    rs_sensor.set_option(RS2_OPTION_DIGITAL_GAIN, 0);
                    configurations[option].current.status_type = OPTION_AUTO;
                }
                else {
                    rs_sensor.set_option(RS2_OPTION_DIGITAL_GAIN, value.value);
                    configurations[option].current.value = value.value;
                }
            }
            break;
        case CameraDevice::DEVICE_LIGHT:
            if (!is_led) {
                if (value.status_type == OPTION_AUTO) {
                     rs_sensor.set_option(RS2_OPTION_EMITTER_ENABLED, 1);
                    configurations[option].current.status_type = OPTION_AUTO;
                }
                else if (value.value == 0) {
                    rs_sensor.set_option(RS2_OPTION_LASER_POWER, 0);
                    configurations[option].current.value = value.value;
                }
                else {
                    float value_o = value.value / configurations[option].scaled_factor;
                    if (configurations[option].step > 1) {
                        if (value.value % configurations[option].step != 0) {
                            float s = (float)(value.value - configurations[option].min) / configurations[option].step;
                            value_o = (roundf(s) * configurations[option].step + configurations[option].min) / configurations[option].scaled_factor;
                        }
                    }
                    rs_sensor.set_option(RS2_OPTION_LASER_POWER, value_o);
                    configurations[option].current.value = value.value;
                }
            }
            else {
                if (value.value == 0) {
                    rs_sensor.set_option(RS2_OPTION_LED_POWER, 0);
                    configurations[option].current.value = value.value;
                }
                else {
                    float value_o = value.value / configurations[option].scaled_factor;
                    if (configurations[option].step > 1) {
                        if (value.value % configurations[option].step != 0) {
                            float s = (float)(value.value - configurations[option].min) / configurations[option].step;
                            value_o = (roundf(s) * configurations[option].step + configurations[option].min) / configurations[option].scaled_factor;
                        }
                    }
                    rs_sensor.set_option(RS2_OPTION_LED_POWER, value_o);
                    configurations[option].current.value = value.value;
                }
            }
            break;
        case DEVICE_CONTRAST:
            set_signle_option_native(option, RS2_OPTION_CONTRAST, value);
            break;
        case DEVICE_HUE:
            set_signle_option_native(option, RS2_OPTION_HUE, value);
            break;
        case DEVICE_SATURATION:
            set_signle_option_native(option, RS2_OPTION_SATURATION, value);
            break;
        case DEVICE_SHARPNESS:
            set_signle_option_native(option, RS2_OPTION_SHARPNESS, value);
            break;
        case DEVICE_GAMMA:
            set_signle_option_native(option, RS2_OPTION_GAMMA, value);
            break;
        case DEVICE_BACKLIGHT:
            set_signle_option_native(option, RS2_OPTION_BACKLIGHT_COMPENSATION, value);
            break;
        case DEVICE_BRIGHTNESS:
            set_signle_option_native(option, RS2_OPTION_BRIGHTNESS, value);
            break;

        }
    }


    CameraStreamRS::CameraStreamRS(const std::string& stream_name,rs2::sensor& rs_sensor, CameraDevice* device) :
        rs_sensor(rs_sensor), CameraStream(stream_name,device)
    {
    }

    CameraProfileRS::CameraProfileRS(rs2::stream_profile& rs_profile, CameraStream* stream) :
        rs_profile(rs_profile), CameraProfile(stream)
    {
        rs2_stream stream_data_type = rs_profile.stream_type();
        rs2::video_stream_profile video_stream_profile = rs_profile.as<rs2::video_stream_profile>();
        format = RS_Profile_to_pixformat(video_stream_profile.format());
        stream_name = rs_profile.stream_name() + ":" + std::string(GET_PIX_TYPE_NAME(format));

        resolution.width = video_stream_profile.width();
        resolution.height = video_stream_profile.height();
        ratio.numerator = video_stream_profile.fps();
        ratio.denominator = 1;
    }

    CameraProfileRS::~CameraProfileRS()
    {
    }

    std::vector<CameraDevice*> EnumerateCamera_RS() {
        rs2::context ctx;
        std::vector<CameraDevice*> devices;
        auto rs_devices = ctx.query_devices();
        for (rs2::device rs_device : rs_devices) {
            auto sensors = rs_device.query_sensors();
            for (rs2::sensor sensor : sensors)
            {
                std::unordered_map<std::string, std::unordered_map<std::string, std::vector<CameraProfile*>>> profile_maps;
                if (sensor.supports(RS2_CAMERA_INFO_NAME) &&
                    (std::string("RGB Camera") == sensor.get_info(RS2_CAMERA_INFO_NAME)
                        || std::string("Stereo Module") == sensor.get_info(RS2_CAMERA_INFO_NAME))) {
                    devices.push_back(new CameraDeviceRS(rs_device, sensor));
                }
            }
        }
        return devices;
    }
};