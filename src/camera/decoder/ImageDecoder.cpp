#include "ImageDecoder.h"
#include <CameraDriver.h>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <frame_types.h>
#include <libyuv.h>
#define RawFrame_CVIMG_(frame) (cv::Mat*)(frame->bgr_frame)


#include <opencv2/core.hpp> 
cv::Mat gamma_map;
void capture::init_postprocess() {
    gamma_map = cv::imread(std_program_path+"data/resources/gamma_precalc.png", cv::IMREAD_GRAYSCALE);
}

void capture::postprocess(RawFrame* frame,Options *transform)
{
    if (!transform->get_option(STREAM_MODE).status_type == OPTION_AUTO) return;
    bool flip_lr = transform->get_option_value(STREAM_FLIP_LR);
    bool flip_ud = transform->get_option_value(STREAM_FLIP_UD);
    int rotate_mode = transform->get_option(STREAM_ROTATE).value;
	int crop_x = transform->get_option_value(STREAM_CROP_X);
	int crop_y = transform->get_option_value(STREAM_CROP_Y);
	unsigned int crop_width = transform->get_option_value(STREAM_CROP_WIDTH);
	unsigned int crop_height = transform->get_option_value(STREAM_CROP_HEIGHT);
    unsigned char*& buffer = (unsigned char*&)transform->buffer;
    bool enable_crop = !(transform->is_default(STREAM_CROP_X) &&
        transform->is_default(STREAM_CROP_Y) &&
        transform->is_default(STREAM_CROP_WIDTH) &&
        transform->is_default(STREAM_CROP_HEIGHT));
    cv::Mat image_temp;
    bool need_drop = false;
    unsigned int width = RawFrame_WIDTH_(frame);
    unsigned int height = RawFrame_HEIGHT_(frame);
    auto format = RawFrame_FORMAT_(frame);
    int rot;
	bool is_lossless = (transform->get_option(STREAM_MODE).value == STREAM_OPTION_LOSSLESS);
    if (is_lossless) {
        if (!enable_crop && (!(flip_lr || flip_ud || rotate_mode != 0) || flip_lr && flip_ud && rotate_mode == 180))
            return;

        switch (format) {
        case PIX_TYPE_BGR8:  image_temp = cv::Mat(height, width, CV_8UC3, frame->raw_frame); goto cv_proc;
        case PIX_TYPE_RGB8:  image_temp = cv::Mat(height, width, CV_8UC3, frame->raw_frame); goto cv_proc;
        case PIX_TYPE_RGBA:  image_temp = cv::Mat(height, width, CV_8UC4, frame->raw_frame); goto cv_proc;
        case PIX_TYPE_BGRA:  image_temp = cv::Mat(height, width, CV_8UC4, frame->raw_frame); goto cv_proc;
        case PIX_TYPE_L8:  image_temp = cv::Mat(height, width, CV_8UC1, frame->raw_frame); goto cv_proc;
        case PIX_TYPE_D16:
        case PIX_TYPE_Z16: image_temp = cv::Mat(height, width, CV_16UC1, frame->raw_frame); goto cv_proc;

        case PIX_TYPE_RGB5:  cv::cvtColor(cv::Mat(height, width, CV_8UC2, frame->raw_frame), image_temp, cv::COLOR_BGR5552RGB); goto cv_img_drop;
        case PIX_TYPE_BGR5:  cv::cvtColor(cv::Mat(height, width, CV_8UC2, frame->raw_frame), image_temp, cv::COLOR_BGR5552BGR); goto cv_img_drop;
        case PIX_TYPE_RGB6:  cv::cvtColor(cv::Mat(height, width, CV_8UC2, frame->raw_frame), image_temp, cv::COLOR_BGR5552RGB); goto cv_img_drop;
        case PIX_TYPE_BGR6:  cv::cvtColor(cv::Mat(height, width, CV_8UC2, frame->raw_frame), image_temp, cv::COLOR_BGR5552BGR); goto cv_img_drop;

        case PIX_TYPE_YUY2:
        case PIX_TYPE_UYVY:
        {
            unsigned char* c[3];
            rot = flip_lr ? (rotate_mode + 180) % 360 : rotate_mode;
            if (rot != 0) {
                if (buffer == nullptr || transform->buffer_len < width * height * 2) {
                    free(buffer); 
                    transform->buffer_len = width * height * 3;
                    buffer = (unsigned char*)malloc(width * height * 3);
                }
                c[0] = buffer;
            }
            else
                c[0] = (unsigned char*)malloc(crop_width * crop_height * 2);
            c[1] = c[0] + (crop_width * crop_height);
            c[2] = c[1] + (crop_width * crop_height / 2);

            auto src = frame->raw_frame + (width * crop_y + crop_x) * 2;
            if (format == PIX_TYPE_YUY2) {
                libyuv::YUY2ToI422(src, width * 2,
                    c[0], crop_width, c[1], crop_width / 2, c[2], crop_width / 2,
                    crop_width, (flip_ud != flip_lr) ? -crop_height : crop_height);

            }
            else if (format == PIX_TYPE_UYVY) {
                libyuv::UYVYToI422(src, width * 2,
                    c[0], crop_width, c[1], crop_width / 2, c[2], crop_width / 2,
                    crop_width, (flip_ud != flip_lr) ? -crop_height : crop_height);
            }

            if (rot % 180 != 0) {
                height = crop_width; width = crop_height;
            }
            else {
                width = crop_width;
                height = crop_height;
            }
            if (rot != 0) {
                unsigned char* d[3];
                d[0] = frame->raw_frame;
                d[1] = d[0] + (width * height);
                d[2] = d[1] + (width * height / 2);
                libyuv::I422Rotate(c[0], crop_width, c[1], crop_width / 2, c[2], crop_width / 2,
                    d[0], width, d[1], width / 2, d[2], width / 2,
                    crop_width, crop_height, (libyuv::RotationModeEnum)rot);
            }
            else {
                if (frame->free_funcs.size() > 0) {
                    frame->free_funcs.front()();
                    frame->free_funcs.pop();
                }
                frame->raw_frame = c[0];
                frame->free_funcs.push([c] {free(c[0]); });
            }
            frame->format = PIX_TYPE_I422;
            frame->resolution = Resolution{ width,height };

        } break;

        case PIX_TYPE_Y12I:
        case PIX_TYPE_NV12: {
            rot = flip_lr ? (rotate_mode + 180) % 360 : rotate_mode;
            if (rot % 180 != 0) {
                height = crop_width; width = crop_height;
            }
            else {
                width = crop_width; height = crop_height;
            }
            unsigned char* d[3];
            d[0] = (unsigned char*)malloc(1.5 * width * height);
            d[1] = d[0] + (width * height);
            d[2] = d[1] + (width * height / 4);
            if (format == PIX_TYPE_NV12)
                libyuv::ConvertToI420(frame->raw_frame, frame->raw_frame_len,
                    d[0], width, d[1], width / 2, d[2], width / 2,
                    crop_x, crop_y,
                    RawFrame_WIDTH_(frame), (flip_ud != flip_lr) ? -RawFrame_HEIGHT_(frame) : RawFrame_HEIGHT_(frame),
                    crop_width, crop_height, (libyuv::RotationModeEnum)rot, libyuv::FOURCC_NV12);
            else if (format == PIX_TYPE_Y12I) {
                libyuv::ConvertToI420(frame->raw_frame, frame->raw_frame_len,
                    d[0], width, d[1], width / 2, d[2], width / 2,
                    crop_x, crop_y,
                    RawFrame_WIDTH_(frame), (flip_ud != flip_lr) ? -RawFrame_HEIGHT_(frame) : RawFrame_HEIGHT_(frame),
                    crop_width, crop_height, (libyuv::RotationModeEnum)rot, libyuv::FOURCC_I420);
            }
            frame->resolution = Resolution{ width,height };
            frame->format = PIX_TYPE_I420;
            if (frame->free_funcs.size() > 0) {
                frame->free_funcs.front()();
                frame->free_funcs.pop();
            }
            frame->raw_frame = d[0];
            frame->free_funcs.push([d] {free(d[0]); });
        }break;
        default:;
        };
        return;
    }
    else {
        image_temp = decode_bgr(frame);
    }
    cv_img_drop:
        frame->format = PIX_TYPE_BGR8;
    cv_proc:
        if (enable_crop) {
            cv::Rect roi(crop_x, crop_y, crop_width, crop_height);
            image_temp = image_temp(roi);
            width = crop_width;
            height = crop_height;
        }
        if (flip_ud != flip_lr)
            cv::flip(image_temp, image_temp, flip_lr ? 1 : 0);
        if (flip_ud && flip_lr)
            rot = (rotate_mode + 180) % 360;
        else
            rot = rotate_mode;
        if (is_lossless) {
            if (rot != 0)
                cv::rotate(image_temp, image_temp, rot / 90 - 1);
            if (rot % 180 != 0)
                std::swap(width, height);
        }
        else {
            //loosy
            if (!transform->is_default(STREAM_CONTRAST) ||
                !transform->is_default(STREAM_BRIGHTNESS)) {
                float contrast = transform->get_option_value(STREAM_CONTRAST);
                int brightness = transform->get_option(STREAM_BRIGHTNESS).value;
                cv::convertScaleAbs(image_temp, image_temp, contrast, brightness);
            }
            if (!transform->is_default(STREAM_SHARPNESS)) {
                double sharpness = transform->get_option_value(STREAM_SHARPNESS);
                if (sharpness > 0) {
                    if (buffer == nullptr || transform->buffer_len < width * height * 3) {
                        free(buffer);
                        transform->buffer_len = width * height * 3;
                        buffer = (unsigned char*)malloc(width * height * 3);
                    }
                    cv::Mat blurredImage(height,width,CV_8UC3, buffer);
                    cv::GaussianBlur(image_temp, blurredImage, cv::Size(0, 0), sharpness);

                    cv::Mat sharpenedImage;
                    cv::addWeighted(image_temp, 1.5, blurredImage, -0.5, 0, image_temp);
                }
                else
                    cv::GaussianBlur(image_temp, image_temp, cv::Size(0, 0), -sharpness);
            }
            if (!transform->is_default(STREAM_SATURATION) ||
                !transform->is_default(STREAM_VALUE)) {
                float saturation_factor = transform->get_option_value(STREAM_SATURATION);
                auto value_change = transform->get_option(STREAM_VALUE);

                if (buffer == nullptr || transform->buffer_len < width * height * 3) {
                    free(buffer);
                    transform->buffer_len = width * height * 3;
                    buffer = (unsigned char*)malloc(width * height * 3);
                }
                cv::Mat hsv(height, width, CV_8UC3, buffer);
                cv::cvtColor(image_temp, hsv, cv::COLOR_BGR2HSV);

                std::vector<cv::Mat> hsv_channels(3);
				hsv_channels[0] = cv::Mat(image_temp.size(), CV_8U, image_temp.data);
                hsv_channels[1] = cv::Mat(image_temp.size(), CV_8U, image_temp.data+ image_temp.size().area());
                hsv_channels[2] = cv::Mat(image_temp.size(), CV_8U, image_temp.data+ 2*image_temp.size().area());
                cv::split(hsv, hsv_channels);
                if(saturation_factor !=1)
				convertScaleAbs(hsv_channels[1], hsv_channels[1], saturation_factor, 0);
                if (value_change.status_type == OPTION_AUTO) {
                    cv::equalizeHist(hsv_channels[2], hsv_channels[2]);
                }
                else if (value_change.value != 0)
                    convertScaleAbs(hsv_channels[2], hsv_channels[2], 1, value_change.value);
                cv::merge(hsv_channels, hsv); 
                cv::cvtColor(hsv, image_temp, cv::COLOR_HSV2BGR);
            }
            if (!transform->is_default(STREAM_GAMMA)) {
                auto gamma = transform->get_option(STREAM_GAMMA);
                if (gamma.status_type != OPTION_AUTO) {
                    auto gamma_lut = gamma_map.row(gamma.value);
                    cv::LUT(image_temp, gamma_lut, image_temp);
                }
                else {
                    if (buffer == nullptr || transform->buffer_len < width * height * 3) {
                        free(buffer);
                        transform->buffer_len = width * height * 3;
                        buffer = (unsigned char*)malloc(width * height * 3);
                    }
                    cv::Mat lab(height, width, CV_8UC3, buffer);
                    cv::cvtColor(image_temp, lab, cv::COLOR_BGR2YUV);
                    std::vector<cv::Mat> lab_channels(3);
                    lab_channels[0] = cv::Mat(image_temp.size(), CV_8U, image_temp.data);
                    lab_channels[1] = cv::Mat(image_temp.size(), CV_8U, image_temp.data + image_temp.size().area());
                    lab_channels[2] = cv::Mat(image_temp.size(), CV_8U, image_temp.data + 2 * image_temp.size().area());
                    cv::split(lab, lab_channels);

                    auto clahe = cv::createCLAHE(2.0, { 8, 8 });
                    clahe->apply(lab_channels[0], lab_channels[0]);

                    cv::merge(lab_channels, lab);
                    cv::cvtColor(lab, image_temp, cv::COLOR_YUV2BGR);
                }
            }
            if (rot != 0) {
                cv::Point2f center((width - 1) / 2.0, (height - 1) / 2.0);
                cv::Mat rot_mat = cv::getRotationMatrix2D(center, rot, 1.0);
                cv::warpAffine(image_temp, image_temp, rot_mat, image_temp.size());
            }
        }

        frame->resolution = Resolution{ width,height };
        if (frame->raw_frame != image_temp.data) {
            if (frame->free_funcs.size() > 0) {
                frame->free_funcs.front()();
                frame->free_funcs.pop();
            }
            frame->raw_frame = image_temp.data;
        }            
        frame->bgr_frame = new cv::Mat(image_temp);
        frame->free_funcs.push([frame]() {delete (cv::Mat*)frame->bgr_frame; });

        return;
}

cv::Mat capture::decode_bgr(RawFrame* frame)
{
    unsigned int width = RawFrame_WIDTH_(frame);
    unsigned int height = RawFrame_HEIGHT_(frame);
    cv::Mat out_image;
    if (RawFrame_FORMAT_(frame) != PIX_TYPE_BGR8)
        out_image = cv::Mat(width, height, CV_8UC3);
    switch (RawFrame_FORMAT_(frame)) {
    case PIX_TYPE_BGR8:  out_image = cv::Mat(height, width, CV_8UC3, frame->raw_frame); break;
    case PIX_TYPE_RGB8:  cv::cvtColor(cv::Mat(height, width, CV_8UC3, frame->raw_frame), out_image, cv::COLOR_RGB2BGR); break;
    case PIX_TYPE_RGBA:  cv::cvtColor(cv::Mat(height, width, CV_8UC4, frame->raw_frame), out_image, cv::COLOR_RGBA2BGR); break;
    case PIX_TYPE_BGRA:  cv::cvtColor(cv::Mat(height, width, CV_8UC4, frame->raw_frame), out_image, cv::COLOR_BGRA2BGR); break;

    case PIX_TYPE_RGB5:  cv::cvtColor(cv::Mat(height, width, CV_8UC2, frame->raw_frame), out_image, cv::COLOR_BGR5552RGB); break;
    case PIX_TYPE_BGR5:  cv::cvtColor(cv::Mat(height, width, CV_8UC2, frame->raw_frame), out_image, cv::COLOR_BGR5552BGR); break;
    case PIX_TYPE_RGB6:  cv::cvtColor(cv::Mat(height, width, CV_8UC2, frame->raw_frame), out_image, cv::COLOR_BGR5552RGB); break;
    case PIX_TYPE_BGR6:  cv::cvtColor(cv::Mat(height, width, CV_8UC2, frame->raw_frame), out_image, cv::COLOR_BGR5552BGR); break;

    case PIX_TYPE_YUY2:  cv::cvtColor(cv::Mat(height, width, CV_8UC2, frame->raw_frame), out_image, cv::COLOR_YUV2BGR_YUY2); break;
    case PIX_TYPE_UYVY:  cv::cvtColor(cv::Mat(height, width, CV_8UC2, frame->raw_frame), out_image, cv::COLOR_YUV2BGR_UYVY); break;
    case PIX_TYPE_I422: {
        out_image = cv::Mat(height, width, CV_8UC3);
        libyuv::I422ToRGB24(frame->raw_frame, width,
            frame->raw_frame+ height * width, width /2,
            frame->raw_frame+int(height * width *1.5), width / 2,
            out_image.data, width *3, width, height
	        );
    }break;

    case PIX_TYPE_NV12:  cv::cvtColor(cv::Mat(height * 1.5, width, CV_8UC1, frame->raw_frame), out_image, cv::COLOR_YUV2BGR_NV12); break;
    case PIX_TYPE_Y12I:  cv::cvtColor(cv::Mat(height * 1.5, width, CV_8UC1, frame->raw_frame), out_image, cv::COLOR_YUV2BGR_I420); break;
    case PIX_TYPE_I420:  cv::cvtColor(cv::Mat(height * 1.5, width, CV_8UC1, frame->raw_frame), out_image, cv::COLOR_YUV2BGR_I420); break;

    case PIX_TYPE_L8:  cv::cvtColor(cv::Mat(height, width, CV_8UC1, frame->raw_frame), out_image, cv::COLOR_GRAY2BGR); break;
    case PIX_TYPE_L16: cv::extractChannel(cv::Mat(height, width, CV_8UC2, frame->raw_frame), out_image, 0); break;
    case PIX_TYPE_D16:
    case PIX_TYPE_Z16:
    {
        cv::Mat temp_img(height, width, CV_16UC1, frame->raw_frame);
        cv::Mat log_img;
        temp_img.convertTo(log_img, CV_8UC1, 0.05);
        cv::applyColorMap(log_img, out_image, cv::COLORMAP_RAINBOW);

    }break;
    case PIX_TYPE_MJPG:  cv::imdecode(cv::Mat(frame->raw_frame_len, 1, CV_8UC1, frame->raw_frame), cv::IMREAD_COLOR, &out_image); break;
    default: return cv::Mat();
    };

    return out_image;
}

cv::Mat capture::decode_yuv(RawFrame* image)
{

	return cv::Mat();
}
