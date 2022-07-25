#ifndef SIGNAL_PROCESS_H
#define SIGNAL_PROCESS_H
#include <QThread>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <qimage.h>
#include <queue>
#include "filter.h"
#include <qlabel>
#include "window.h"
#include <atomic>

#define FS 100 //hz

class SignalProcess : public QThread
{
    Q_OBJECT

public:
    SignalProcess(QLabel* videoLabel);

    Q_SIGNAL void interpRGBReady(float, float, float, double, float*);
    Q_SIGNAL void interpPPGReady(double, double);
    Q_SIGNAL void rqiReady(double, double, double);
    Q_SIGNAL void fftReady(QImage);
    Q_SLOT void processSignal(cv::Scalar,double);
    Q_SLOT void  processPPG(uint16_t ppg, double ts);
    Q_SLOT void reset();
    Q_SLOT void setShowR(bool);
    Q_SLOT void setShowG(bool);
    Q_SLOT void setShowB(bool);
    void setParameters(double ,double,double,double);
    std::atomic<double> cam_ofs = 0;
    std::atomic<double> win_length = 6;
private:
    double current_ts = 0;
    double previous_rgb_ts = 0;
    cv::Scalar previous_mean_BGR;
    Window<float>* window_ppg[4];


    double previous_ppg_ts;
    uint16_t previous_ppg;
    double current_ts_ppg = 0;
    double last_rqi_ts = 0;

    QLabel* videoLabel;
    cv::Mat filter_a;
    cv::Mat filter_b;
    cv::Mat canvas;
    cv::Mat hanning_window;

    std::vector<cv::Point>* fftPoints;
    std::vector<cv::Point> timePoints;
    //Window<double>* channel;
    bool show_r = false;
    bool show_g = true;
    bool show_b = false;
    double resolution;
    double fft_front_freq;
    int br_signal_length;
    int phase_slice_time;
    int timeDomain_Length;
    int timeDomain_src_stride;
    int timeDomain_dst_scale;
    int fft_front_index;
    int fft_back_index;
    int fft_filter_length;
    int sample_freq;
    int signal_length;
    int fft_length;
    FilterIIR *filter[4];
    FilterIIR* br_filter;
    unsigned int i_loop;
    int overlap;
    double interp_cyc;
    double fs;
    double last_phase=0;
    int ts0;
    double cam_delay;

    int spectrum_length;
    bool sig_show = true;

    static uint64_t BREATH_COEFF[][6];
    static uint64_t HEART_COEFF[][6];
    static int BREATH_ORDER;
    static int HEART_ORDER;

    std::vector<double> phase_delay;
};

#endif // SIGNAL_PROCESS_H
