#include "signalprocess.h"
#include <vector>
#include <qdebug.h>
#include <QPixmap>
#include <future>
#include <execution>
using namespace cv;
using namespace std;


uint64_t SignalProcess::BREATH_COEFF[][6] = {
{0x3fa06886aab44487,0xbfa31749a10eb943,0x3fa06886aab44485,0x3ff0000000000000,0xbffa5e303e939420,0x3fe6a4edfef97da0,},
{0x3ff0000000000000,0xbffc739f70a76fe6,0x3ff0000000000000,0x3ff0000000000000,0xbffccf2620fd3656,0x3fed374b4eab4aec,},
{0x3ff0000000000000,0xbfffff9dce6391a8,0x3feffffffffffffe,0x3ff0000000000000,0xbfff37691b105e90,0x3fee79f5a517b207,},
{0x3ff0000000000000,0xbffffe45af359465,0x3ff0000000000001,0x3ff0000000000000,0xbfffdd3eb4bda85a,0x3fefc129d2e654c1,},
};

int SignalProcess::BREATH_ORDER = 8;
int SignalProcess::HEART_ORDER = 10;
uint64_t SignalProcess::HEART_COEFF[][6] = { 
    {0x3eb3d117110c3074,0x3ec3d117110c3074,0x3eb3d117110c3074,0x3ff0000000000000,0xbffd5177efdffb9d,0x3feb5b7839bfa6e5,},
{0x3ff0000000000000,0x4000000000000000,0x3ff0000000000000,0x3ff0000000000000,0xbffdcf4b9a92aab8,0x3febea564af6718e,},
{0x3ff0000000000000,0x0,0xbff0000000000000,0x3ff0000000000000,0xbffe76b8bcf57762,0x3fedfc043424716a,},
{0x3ff0000000000000,0xc000000000000000,0x3ff0000000000000,0x3ff0000000000000,0xbffef3337cc519fe,0x3fee063878219c7d,},
{0x3ff0000000000000,0xc000000000000000,0x3ff0000000000000,0x3ff0000000000000,0xbfffa7e1ad1072c0,0x3fef66e06d61fcb3,}, };

#define MAX_SPECTRUM 600
#define X_SCALE  2
#define interp_cyc 0.01
// double vs. float ??
SignalProcess::SignalProcess(QLabel* videoLabel)
    : overlap(5)
    , filter()
    ,videoLabel(videoLabel)
    , previous_mean_BGR(-1,-1,-1,-1)
{
    window_ppg[0] = new Window<float>(6000, 3000);
    window_ppg[1] = new Window<float>(6000, 3000);
    window_ppg[2] = new Window<float>(6000, 3000);
    window_ppg[3] = new Window<float>(6000, 3000);
    filter[0] = new FilterIIR(HEART_COEFF, HEART_ORDER);
    filter[1] = new FilterIIR(HEART_COEFF, HEART_ORDER);
    filter[2] = new FilterIIR(HEART_COEFF, HEART_ORDER);
    filter[3] = new FilterIIR(HEART_COEFF, HEART_ORDER);

    fs = 1000.0 / 10;
    signal_length = fs * 6; //15s
    i_loop = 0;

    phase_slice_time = 5;
    br_signal_length = 30;

    fft_length = 6000; // 1bpm resolution
    resolution = fs/fft_length;
    fft_front_index = 50.0 / 60 / resolution;  // 52bpm
    fft_back_index = 200.0 / 60 / resolution;
    fft_front_freq = resolution * fft_front_index;
    double fft_back_freq = resolution * fft_back_index;
    fft_filter_length = fft_back_index - fft_front_index;


    spectrum_length = fs * 30 / overlap; // 20s

    //channel = new Window<double>(fs * 3 / 2 * 3*10,fs*3/*channels*/ / 2/*overlap*/*3/*seconds*/);   //3s
    timeDomain_Length = fs * 4;
    timeDomain_src_stride = 4;
    timeDomain_dst_scale = 2;
    fftPoints = new std::vector<cv::Point>(fft_filter_length);
    timePoints.reserve(timeDomain_Length / timeDomain_src_stride);

    ts0 = 0;


    // Estimated heartrate:
    int estimated_heartrate = (70.0 / 60 - fft_front_freq) / resolution;
    int estimated_min_heartrate = (60.0 / 60 - fft_front_freq) / resolution;
    int estimated_max_heartrate = (120.0 / 60 - fft_front_freq) / resolution;

    canvas = Mat(MAX_SPECTRUM, fft_filter_length * X_SCALE, CV_8UC3, Scalar_<uint8_t>(255, 255, 255));
    for (int i = 55; i < 200; i += 5) {
        int x_pos = (int((double)i / 60 / resolution) - fft_front_index) * X_SCALE;
        if (i % 10 == 0) {
            cv::putText(canvas, QString::number(i).toStdString(), Point2i(x_pos, MAX_SPECTRUM - 3), cv::FONT_HERSHEY_SIMPLEX, 0.25, Scalar_<uint8_t>(0, 0, 0));
            cv::line(canvas, Point2i(x_pos, MAX_SPECTRUM), Point2i(x_pos, MAX_SPECTRUM- 5), Scalar_<uint8_t>(0, 0, 0));
        }
        else

            cv::line(canvas, Point2i(x_pos, MAX_SPECTRUM), Point2i(x_pos, MAX_SPECTRUM- 1), Scalar_<uint8_t>(0, 0, 0));
    }
    for (int i = 0; i < 600; i += 25) {
        cv::putText(canvas, QString::number(600-i).toStdString(), Point2i(0, i+1), cv::FONT_HERSHEY_SIMPLEX, 0.25, Scalar_<uint8_t>(0, 0, 0));
        cv::line(canvas, Point2i(0, i), Point2i(5, i), Scalar_<uint8_t>(0, 0, 0));
    }
}
void SignalProcess::processSignal(cv::Scalar bgr_signal, double ts) {
    static std::vector<cv::Point>* fftPoints[3] = { new std::vector<cv::Point>(fft_filter_length),new std::vector<cv::Point>(fft_filter_length),new std::vector<cv::Point>(fft_filter_length) };
    static std::vector<uint8_t> itor_rgb = { 0,1,2 };
    static Mat canvas_spectrum;
    if (current_ts == 0) {
        int64_t ts_temp = ts * 1000;
        current_ts = (double)(ts_temp - ts_temp % 10) / 1000;
        previous_mean_BGR = bgr_signal;
        previous_rgb_ts = ts;
    }
    else {
        while (ts > current_ts + interp_cyc) {
            current_ts += interp_cyc;
            Scalar interp_bgr = previous_mean_BGR + (bgr_signal - previous_mean_BGR) / (ts - previous_rgb_ts) * (current_ts - previous_rgb_ts);
            float raw_b = filter[0]->filter(interp_bgr[0]);
            float raw_g = filter[1]->filter(interp_bgr[1]);
            float raw_r = filter[2]->filter(interp_bgr[2]);
            window_ppg[0]->shift(raw_r);
            window_ppg[1]->shift(raw_g);
            window_ppg[2]->shift(raw_b);
            cv::Mat ppg_vec[4];
            int win_len = win_length*100;
            if (current_ts_ppg - last_rqi_ts > 0.1) {
                last_rqi_ts = current_ts_ppg;
                int ts_ofs = round((current_ts_ppg - current_ts) * 100+ cam_ofs*100);
                if (ts_ofs > 0) {
                    ppg_vec[3] = cv::Mat(win_len, 1, CV_32F, window_ppg[3]->backp - win_len - ts_ofs);
                    ppg_vec[0] = cv::Mat(win_len, 1, CV_32F, window_ppg[0]->backp - win_len);
                    ppg_vec[1] = cv::Mat(win_len, 1, CV_32F, window_ppg[1]->backp - win_len);
                    ppg_vec[2] = cv::Mat(win_len, 1, CV_32F, window_ppg[2]->backp - win_len);
                }
                else {
                    ppg_vec[3] = cv::Mat(win_len, 1, CV_32F, window_ppg[3]->backp - win_len);
                    ppg_vec[0] = cv::Mat(win_len, 1, CV_32F, window_ppg[0]->backp - win_len + ts_ofs);
                    ppg_vec[1] = cv::Mat(win_len, 1, CV_32F, window_ppg[1]->backp - win_len + ts_ofs);
                    ppg_vec[2] = cv::Mat(win_len, 1, CV_32F, window_ppg[2]->backp - win_len + ts_ofs);
                }
                float* sqi = new float[6];
                Mat norm_ppg;
                cv::normalize(ppg_vec[3], norm_ppg, 1.0, -1.0, NORM_MINMAX);

                int y_true_max = videoLabel->height();
                Mat(canvas, Rect2i(0,MAX_SPECTRUM- y_true_max, canvas.cols, y_true_max)).copyTo(canvas_spectrum);
                Mat ppg_fft_padding = Mat::zeros(fft_length, 1, CV_32F);
                std::memcpy(ppg_fft_padding.data, norm_ppg.data, 4 * norm_ppg.rows);
                Mat fft_out;
                cv::dft(ppg_fft_padding, fft_out);
                const float* fft_p = fft_out.ptr<float>(fft_front_index*2);
                int max_idx = 0, max_val = 0;
                for (int i = 0; i < fft_filter_length; i++) {
                    double val = fft_p[0] * fft_p[0] + fft_p[1] * fft_p[1];
                    if (max_val < val) {
                        max_val = val;
                        max_idx = i;
                    }
                    fft_p += 2;
                }
                int hormonic_idx = ((max_idx + fft_front_index) * 2 - fft_front_index) ;



                Mat fft_padding[3];// = { Mat::zeros(fft_length, 1, CV_64F),Mat::zeros(fft_length, 1, CV_64F),Mat::zeros(fft_length, 1, CV_64F) };
                Mat spectrum[3];
                //std::vector<double> vec = fft_padding;
                //cnpy::npy_save("E:/avaa/heartRate/filter/sig.npy", vec);

                float sig_power[3] = { 0,0,0 }, all_power[3] = {0,0,0};
                std::for_each(std::execution::par,
                    itor_rgb.begin(), itor_rgb.end(),
                    [&](auto i_rgb) {
                        sqi[i_rgb] = cv::sum(norm_ppg.mul(ppg_vec[i_rgb]))[0];
                        fft_padding[i_rgb] = Mat::zeros(fft_length, 1, CV_32F);
                        memcpy(fft_padding[i_rgb].data, ppg_vec[i_rgb].data, 4 * ppg_vec[i_rgb].rows);
                        Mat fft_out;
                        dft(fft_padding[i_rgb], fft_out);
                        spectrum[i_rgb] = Mat (fft_filter_length, 1, CV_32F);
                        const float* fft_p = fft_out.ptr<float>(fft_front_index*2);
                        float* spec_p = spectrum[i_rgb].ptr<float>(0);
                        for (int i = 0; i < fft_filter_length; i++) {
                            *spec_p = sqrt(fft_p[0] * fft_p[0] + fft_p[1] * fft_p[1]);
                            all_power[i_rgb] += *spec_p;
                            if (abs(i - max_idx) < 5 ){//|| abs(i - hormonic_idx) < 3) {
                                sig_power[i_rgb] += *spec_p;
                            }
                            fftPoints[i_rgb]->operator[](i).y = y_true_max -*spec_p;
                            fftPoints[i_rgb]->operator[](i).x = i * 2;
                            spec_p += 1;
                            fft_p += 2;
                        }
                        sqi[i_rgb+3] = sig_power[i_rgb] / all_power[i_rgb];
                    });
                emit interpRGBReady(raw_r, raw_g, raw_b, current_ts, sqi);
                if (show_r)
                polylines(canvas_spectrum, *(fftPoints[0]), false, Scalar_<uint8_t>(0, 0, 255));
                if (show_g)
                polylines(canvas_spectrum, *(fftPoints[1]), false, Scalar_<uint8_t>(0, 255, 0));
                if (show_b)
                polylines(canvas_spectrum, *(fftPoints[2]), false, Scalar_<uint8_t>(255, 0, 0));

                cv::line(canvas_spectrum, Point2i(max_idx * 2, 0), Point2i(max_idx * 2, y_true_max), Scalar_<uint8_t>(0, 0, 0),2);
                cv::line(canvas_spectrum, Point2i(hormonic_idx*2, 0), Point2i(hormonic_idx*2, y_true_max), Scalar_<uint8_t>(0, 0, 0),1);

                //auto p_canvas_spectrum = QSharedPointer<QImage>(new QImage(canvas_spectrum.data,
                //    fft_filter_length * 2, y_true_max * 2,
                //    fft_filter_length * 6, QImage::Format_BGR888), [](QImage* obj) {
                //        delete obj;
                //    });
                emit fftReady(QImage(canvas_spectrum.data,
                    fft_filter_length * 2, y_true_max,
                    fft_filter_length * 6, QImage::Format_BGR888));


            }else
                emit interpRGBReady(raw_r, raw_g, raw_b, current_ts,NULL);
        }
        previous_mean_BGR = bgr_signal;
        previous_rgb_ts = ts;
    }
}
void SignalProcess::setShowR(bool isChecked) {
    show_r = isChecked;
}
void SignalProcess::setShowG(bool isChecked) {
    show_g = isChecked;
}
void SignalProcess::setShowB(bool isChecked) {
    show_b = isChecked;
}
void SignalProcess::processPPG(uint16_t ppg, double ts) {
    if (current_ts_ppg == 0) {
        int64_t ts_temp = ts * 1000;
        current_ts_ppg = (double)(ts_temp - ts_temp % 10) / 1000;
        previous_ppg_ts = ts;
        previous_ppg = ppg;
        last_rqi_ts = current_ts_ppg;
    }
    else {
        while (ts > current_ts_ppg + interp_cyc) {
            current_ts_ppg += interp_cyc;
            double raw_ppg = -filter[3]->filter((double)previous_ppg + (double)(previous_ppg - ppg) / (previous_ppg_ts - ts) * (previous_ppg_ts - current_ts_ppg));
            emit interpPPGReady(raw_ppg, current_ts_ppg);
            window_ppg[3]->shift(raw_ppg);
        }
        previous_ppg_ts = ts;
        previous_ppg = ppg;
    }
}
void SignalProcess::reset() {
    filter[0]->reset();
    filter[1]->reset();
    filter[2]->reset();
    filter[3]->reset();
    current_ts = 0;
    current_ts_ppg = 0;
    window_ppg[0]->reset();
    window_ppg[1]->reset();
    window_ppg[2]->reset();
    window_ppg[3]->reset();
}
