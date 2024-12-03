#ifndef SIGNAL_PROCESS_H
#define SIGNAL_PROCESS_H
#include <QThread>
#include <opencv2/imgproc.hpp>
#include <qimage.h>
#include <queue>
#include <qlabel>
#include <atomic>

#include<qcustomplot.h>
#include<unordered_map>
#include<unordered_set>
class OverlapAdding;
class SignalProcess : public QThread
{
    Q_OBJECT

public:
    SignalProcess(QLabel* videoLabel);

    Q_SIGNAL void sqiReady(int channel,float, float);
    Q_SIGNAL void fftReady(const QImage&);
    Q_SLOT void processSignal(cv::Scalar,double);
    Q_SLOT void  processPPG(double ppg, double ts);
    Q_SLOT void reset_ppg();
    Q_SLOT void reset_rppg();
    Q_SLOT void setShowChannel(bool);
    std::atomic<double> cam_ofs = 0;
private:
    double current_ts = 0;
    double current_ts_ppg = 0;

    double previous_bgr_ts = 0;
    cv::Scalar previous_bgr;
    double previous_ppg_ts;
    double previous_ppg;
    unsigned i_loop=0;
    unsigned i_loop_ppg = 0;

    QLabel* videoLabel;
    cv::Mat canvas;
    cv::Mat point_list_mat;

    int ref_channel;

    bool show_r = false;
    bool show_g = false;
    bool show_b = false;
    bool show_pos = true;
    bool show_ref = false;


    QVector<QCPGraphData> graph_pos_end_data;
    cv::Mat line_space_quick_copy;
    void show_channel(int channel, QCPGraph* graph,bool is_show);
public:
    QCPGraph* graph_r, * graph_g, * graph_b, * graph_pos, * graph_pos_end, * graph_ppg;
    int r_channel;
    int g_channel;
    int b_channel;
    int pos_channel;
};

#endif // SIGNAL_PROCESS_H
