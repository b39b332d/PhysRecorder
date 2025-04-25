#include "signalprocess.h"
#include <vector>
#include <qdebug.h>
#include <QPixmap>

#include "RPPGExtractor.h"

#define MAX_SPECTRUM 600
#define X_SCALE  2
#define Y_SCALE  5
extern QCPGraph* graph_r, * graph_g, * graph_b, * graph_pos, * graph_pos_end, * graph_ppg;

extern double win_length;
SignalProcess::SignalProcess(QLabel* videoLabel)
    :videoLabel(videoLabel)
    , previous_bgr(-1,-1,-1,-1)
    , graph_pos_end_data(POS_WIN_ON_RGB_GRAPH + 1)
    , line_space_quick_copy(MAX_WINDOWS_LEN/ UPDATE_RGB_GRAPH_STRIDE,1,CV_64F)
{
    i_loop = 0;
    canvas = cv::Mat(MAX_SPECTRUM, FFT_ROI_LENGTH * X_SCALE, CV_8UC3, cv::Scalar_<uint8_t>(255, 255, 255));
    for (int i = int(FREQ_ROI_MIN_BPM/FREQ_ROI_RESOLUTION)* FREQ_ROI_RESOLUTION + FREQ_ROI_RESOLUTION; i < FREQ_ROI_MAX_BPM; i += FREQ_ROI_RESOLUTION) {
        int x_pos = (int((double)i / FFT_RESOLUTION_BPM) - FFT_ROI_MIN) * X_SCALE+ X_SCALE-1;
        if (i % 10 == 0) {
            cv::putText(canvas, QString::number(i).toStdString(), cv::Point2i(x_pos, MAX_SPECTRUM - 3), cv::FONT_HERSHEY_SIMPLEX, 0.25, cv::Scalar_<uint8_t>(0, 0, 0));
            cv::line(canvas, cv::Point2i(x_pos, MAX_SPECTRUM), cv::Point2i(x_pos, MAX_SPECTRUM- 5), cv::Scalar_<uint8_t>(0, 0, 0));
        }
        else
            cv::line(canvas, cv::Point2i(x_pos, MAX_SPECTRUM), cv::Point2i(x_pos, MAX_SPECTRUM- 1), cv::Scalar_<uint8_t>(0, 0, 0));
    }
    point_list_mat = cv::Mat(FFT_ROI_LENGTH, 2, CV_32S);
    int v = X_SCALE / 2;
    for (int i =0 ; i < FFT_ROI_LENGTH; i++) {
        point_list_mat.at<int>(i, 0) = v;
        v += X_SCALE;
    }
    for (int i = 0; i < 600; i += 25) {
        cv::putText(canvas, QString::number((600 - i) / Y_SCALE).toStdString(), cv::Point2i(0, i + 1), cv::FONT_HERSHEY_SIMPLEX, 0.25, cv::Scalar_<uint8_t>(0, 0, 0));
        cv::line(canvas, cv::Point2i(0, i), cv::Point2i(5, i), cv::Scalar_<uint8_t>(0, 0, 0));
    }
    r_channel = RPPGExtractor::create_channel(RPPGExtractor::NORM_CHANNEL);
    g_channel = RPPGExtractor::create_channel(RPPGExtractor::NORM_CHANNEL);
    b_channel = RPPGExtractor::create_channel(RPPGExtractor::NORM_CHANNEL);
    ppg_channel = RPPGExtractor::create_channel(RPPGExtractor::NORM_CHANNEL);
    pos_channel = RPPGExtractor::create_channel(RPPGExtractor::POS_CHANNEL);

    for (int i = 1; i <= POS_WIN_ON_RGB_GRAPH ; i++) {
        graph_pos_end_data[i].key = (i  - POS_WIN_ON_RGB_GRAPH) * UPDATE_RGB_GRAPH_STRIDE_SEC;
    }
    for (int i = 0; i < MAX_WINDOWS_LEN / UPDATE_RGB_GRAPH_STRIDE; i++) {
        line_space_quick_copy.at<double>(i,0) = (i+1 - MAX_WINDOWS_LEN / UPDATE_RGB_GRAPH_STRIDE) * UPDATE_RGB_GRAPH_STRIDE_SEC;
    }
}

void SignalProcess::processSignal(FaceRoi *roi) {
	cv::Scalar bgr_signal(roi->r,roi->g,roi->b);
	double ts = roi->ts;
    delete roi;
    if (current_ts == 0) {
        int64_t ts_temp = ts * 1000;
        current_ts = (double)(ts_temp - ts_temp % 10) / 1000;
        previous_bgr = bgr_signal;
        previous_bgr_ts = ts;
        i_loop = 0;

        graph_r->data()->clear();
        graph_g->data()->clear();
        graph_b->data()->clear();
        graph_pos->data()->clear();
        graph_pos_end->data()->clear();
        RPPGExtractor::reset(r_channel);
        RPPGExtractor::reset(g_channel);
        RPPGExtractor::reset(b_channel);
        RPPGExtractor::reset(pos_channel);
    }
    else {
        while (ts > current_ts + INTERP_CYC) {
            current_ts += INTERP_CYC;
            i_loop++;
            cv::Scalar interp_bgr = previous_bgr + (bgr_signal - previous_bgr) / (ts - previous_bgr_ts) * (current_ts - previous_bgr_ts);

            // negative because rppg is antiphase
            double raw_b = RPPGExtractor::process_signal(b_channel, -interp_bgr[0]);
            double raw_g = RPPGExtractor::process_signal(g_channel, -interp_bgr[1]);
            double raw_r = RPPGExtractor::process_signal(r_channel, -interp_bgr[2]);
            double pos_sig = RPPGExtractor::process_signal(pos_channel, raw_r, raw_g, raw_b);


            if (i_loop % UPDATE_RGB_GRAPH_STRIDE == 0) {

                if (show_r) {
                    graph_r->addData(current_ts, raw_r);
                    graph_r->data()->removeBefore(current_ts - win_length + cam_ofs);
                }
                if (show_g) {
                    graph_g->addData(current_ts, raw_g);
                    graph_g->data()->removeBefore(current_ts - win_length + cam_ofs);
                }

                if (show_b) {
                    graph_b->addData(current_ts, raw_b);
                    graph_b->data()->removeBefore(current_ts - win_length + cam_ofs);
                }

                if (show_pos) {

                    auto pos_end_p_cv = RPPGExtractor::get_signal(pos_channel, POS_WINSIZE,0, UPDATE_RGB_GRAPH_STRIDE);
                    // assert(pos_end_p_cv.rows == POS_WIN_ON_RGB_GRAPH);
                    cv::Mat graph_pos_end_data_cv(POS_WIN_ON_RGB_GRAPH, 2, CV_64F, graph_pos_end_data.data() + 1);
                    pos_end_p_cv.convertTo(graph_pos_end_data_cv.col(1), CV_64F);
                    graph_pos_end_data[0].key = current_ts - POS_WINSIZE_SEC;
                    graph_pos_end_data[0].value = pos_sig;
                    graph_pos_end_data_cv.col(0) += (current_ts - graph_pos_end_data_cv.at<double>(POS_WIN_ON_RGB_GRAPH - 1, 0));

                    graph_pos->addData(current_ts - POS_WINSIZE_SEC, pos_sig);
                    graph_pos_end->data()->set(graph_pos_end_data, true);
                    graph_pos->data()->removeBefore(current_ts - win_length+ cam_ofs);
                }
            }
            if (i_loop % UPDATE_FFT_STRIDE != 0) {
                continue;
            }
            int y_true_max = videoLabel->height();

            cv::Mat *canvas_spectrum = new cv::Mat;
            cv::Mat(canvas, cv::Rect2i(0, MAX_SPECTRUM - y_true_max, canvas.cols, y_true_max)).copyTo(*canvas_spectrum);
            int win_len = win_length * INTERP_FS;
            int max_idx;
            int ref_channel;
            float snr;
            unsigned ofs_rgb = 0, ofs_ref=0;
            if (current_ts_ppg == 0) {
                auto spec = RPPGExtractor::get_spectrum(pos_channel, 0, win_len, &max_idx);
                spec.convertTo(point_list_mat.col(1), CV_32S, -Y_SCALE, y_true_max);
                polylines(*canvas_spectrum, point_list_mat, false, cv::Scalar_<uint8_t>(255, 0, 255));

                int hormonic_idx = (max_idx + FFT_ROI_MIN) * X_SCALE - FFT_ROI_MIN - 1;
                cv::line(*canvas_spectrum, cv::Point2i(max_idx * X_SCALE, 0), cv::Point2i(max_idx * X_SCALE, y_true_max), cv::Scalar_<uint8_t>(0, 0, 0), 2);
                cv::line(*canvas_spectrum, cv::Point2i(hormonic_idx * X_SCALE, 0), cv::Point2i(hormonic_idx * X_SCALE, y_true_max), cv::Scalar_<uint8_t>(0, 0, 0), 1);
                ref_channel = pos_channel;
                }
            else {
                int ts_ofs = round(((current_ts_ppg - current_ts) + cam_ofs) * INTERP_FS);
                ofs_ref = (ts_ofs > 0 ? ts_ofs : 0);
                ofs_rgb = (ts_ofs < 0 ? -ts_ofs : 0);

                auto spec = RPPGExtractor::get_spectrum(ppg_channel, ofs_ref, win_len, &max_idx);
                spec.convertTo(point_list_mat.col(1), CV_32S,  - 15.0*Y_SCALE/ (spec.at<float>(max_idx, 0)+1e-7), y_true_max);
                polylines(*canvas_spectrum, point_list_mat, false, cv::Scalar_<uint8_t>(255, 255, 0));
                int hormonic_idx = (max_idx + FFT_ROI_MIN) * X_SCALE - FFT_ROI_MIN - 1;
                cv::line(*canvas_spectrum, cv::Point2i(max_idx * X_SCALE, 0), cv::Point2i(max_idx * X_SCALE, y_true_max), cv::Scalar_<uint8_t>(0, 0, 0), 2);
                cv::line(*canvas_spectrum, cv::Point2i(hormonic_idx * X_SCALE, 0), cv::Point2i(hormonic_idx * X_SCALE, y_true_max), cv::Scalar_<uint8_t>(0, 0, 0), 1);

                ref_channel = ppg_channel;
                if (show_pos) {
                    auto spec1 = RPPGExtractor::get_spectrum(pos_channel, ofs_rgb, win_len, &max_idx, &snr);
                    spec1.convertTo(point_list_mat.col(1), CV_32S, -Y_SCALE, y_true_max);
                    polylines(*canvas_spectrum, point_list_mat, false, cv::Scalar_<uint8_t>(255, 0, 255));
                    auto sqi = RPPGExtractor::get_pcc(pos_channel, ref_channel, win_len, ofs_rgb, ofs_ref);
                    emit sqiReady(pos_channel, snr, sqi);
                }
            }
            if (show_r) {
                auto spec = RPPGExtractor::get_spectrum(r_channel, ofs_rgb, win_len, &max_idx, &snr);
                spec.convertTo(point_list_mat.col(1), CV_32S, -Y_SCALE, y_true_max);
                polylines(*canvas_spectrum, point_list_mat, false, cv::Scalar_<uint8_t>(0, 0, 255));
                auto sqi = RPPGExtractor::get_pcc(r_channel, ref_channel, win_len, ofs_rgb, ofs_ref);
                emit sqiReady(r_channel, snr, sqi);
            }
            if (show_g) {
                auto spec = RPPGExtractor::get_spectrum(g_channel, ofs_rgb, win_len, &max_idx, &snr);
                spec.convertTo(point_list_mat.col(1), CV_32S, -Y_SCALE, y_true_max);
                polylines(*canvas_spectrum, point_list_mat, false, cv::Scalar_<uint8_t>(0, 255, 0));
                auto sqi = RPPGExtractor::get_pcc(g_channel, ref_channel, win_len, ofs_rgb, ofs_ref);
                emit sqiReady(g_channel, snr, sqi);
            }
            if (show_b) {
                auto spec = RPPGExtractor::get_spectrum(b_channel, ofs_rgb, win_len, &max_idx, &snr);
                spec.convertTo(point_list_mat.col(1), CV_32S, -Y_SCALE, y_true_max);
                polylines(*canvas_spectrum, point_list_mat, false, cv::Scalar_<uint8_t>(255, 0, 0));
                auto sqi = RPPGExtractor::get_pcc(b_channel, ref_channel, win_len, ofs_rgb, ofs_ref);
                emit sqiReady(b_channel, snr, sqi);
            }                


            emit fftReady(QImage(canvas_spectrum->data,
                canvas_spectrum->cols, y_true_max,
                canvas_spectrum->cols * 3, QImage::Format_BGR888, [](void* info) { delete ((cv::Mat*)info); }, canvas_spectrum));

        }
        previous_bgr = bgr_signal;
        previous_bgr_ts = ts;
    }
}
void SignalProcess::show_channel(int channel, QCPGraph* graph, bool is_show) {
    graph->setVisible(is_show);
    if (is_show) {
        double cts = current_ts - INTERP_CYC * (i_loop % UPDATE_RGB_GRAPH_STRIDE);
        if (channel == pos_channel) {
            // get signal before pos
            cv::Mat s = RPPGExtractor::get_signal(channel, win_length * INTERP_FS- POS_WINSIZE, POS_WINSIZE+i_loop % UPDATE_RGB_GRAPH_STRIDE, UPDATE_RGB_GRAPH_STRIDE);
            if (s.rows <= 1)return;
            QVector<QCPGraphData> graph_data(s.rows);
            cv::Mat graph_data_mat(s.rows, 2, CV_64F, graph_data.data());
            // create y value
            s.convertTo(graph_data_mat.col(1), CV_64F);
            // create x value (timestamps)
            cv::Mat(s.rows, 1, CV_64F, (void*)(line_space_quick_copy.dataend - s.rows * sizeof(double))).copyTo(graph_data_mat.col(0));
            graph_data_mat.col(0) += cts - INTERP_CYC*(POS_WINSIZE + i_loop % UPDATE_RGB_GRAPH_STRIDE);
            graph->data()->set(graph_data, true);

            // get signal after pos
            auto pos_end_p_cv = RPPGExtractor::get_signal(pos_channel, POS_WINSIZE, i_loop % UPDATE_RGB_GRAPH_STRIDE, UPDATE_RGB_GRAPH_STRIDE);
            cv::Mat graph_pos_end_data_cv(POS_WIN_ON_RGB_GRAPH, 2, CV_64F, graph_pos_end_data.data() + 1);
            pos_end_p_cv.convertTo(graph_pos_end_data_cv.col(1), CV_64F);
            graph_pos_end_data[0].key = cts - POS_WINSIZE_SEC;
            graph_pos_end_data[0].value = s.at<float>(s.rows - 1,0);
            graph_pos_end_data_cv.col(0) += (cts - graph_pos_end_data_cv.at<double>(POS_WIN_ON_RGB_GRAPH - 1, 0));
            graph_pos_end->data()->set(graph_pos_end_data, true);
            graph_pos_end->setVisible(is_show);
        }
        else {
            cv::Mat s = RPPGExtractor::get_signal(channel, win_length * INTERP_FS, i_loop % UPDATE_RGB_GRAPH_STRIDE, UPDATE_RGB_GRAPH_STRIDE);
            if (s.rows <= 1)return;
            QVector<QCPGraphData> graph_data(s.rows);
            cv::Mat graph_data_mat(s.rows, 2, CV_64F, graph_data.data());
            s.convertTo(graph_data_mat.col(1), CV_64F);
            cv::Mat(s.rows, 1, CV_64F, (void*)(line_space_quick_copy.dataend - s.rows * sizeof(double))).copyTo(graph_data_mat.col(0));
            graph_data_mat.col(0) += cts;
            graph->data()->set(graph_data, true);
        }
    }
    else {
        graph->data()->clear();
        if (channel == pos_channel) {
            graph_pos_end->data()->clear();
            graph_pos_end->setVisible(is_show);
        }
    }
}

void SignalProcess::setShowChannel(bool isChecked) {
    int channel_n = QObject::sender()->property("channel").toInt();
    if (channel_n == r_channel) {
        show_r = isChecked;
        show_channel(r_channel, graph_r, isChecked);
    }else if (channel_n == g_channel) {
        show_g = isChecked;
        show_channel(g_channel, graph_g, isChecked);
    }
    else if (channel_n == b_channel) {
        show_b = isChecked;
        show_channel(b_channel, graph_b, isChecked);
    }
    else if (channel_n == pos_channel) {
        show_pos = isChecked;
        show_channel(pos_channel, graph_pos, isChecked);
    }
    else if (channel_n == ppg_channel) {
        show_pos = isChecked;
        show_channel(ppg_channel, graph_ppg, isChecked);
    }
}

void SignalProcess::processPPG(double ppg, double ts) {
    if (current_ts_ppg == 0 ) {
        int64_t ts_temp = ts * 1000;
        current_ts_ppg = (double)(ts_temp - ts_temp % 10) / 1000;
        previous_ppg_ts = ts;
        previous_ppg = ppg;
        i_loop_ppg = 0;
        RPPGExtractor::reset(ppg_channel);
    }
    else {
        if (ts < current_ts_ppg + INTERP_CYC)previous_ppg = (previous_ppg + ppg) / 2;
        while (ts >= current_ts_ppg + INTERP_CYC) {
            i_loop_ppg++;
            current_ts_ppg += INTERP_CYC;
            double  ref = previous_ppg + (ppg - previous_ppg) / (ts - previous_ppg_ts) * (current_ts_ppg - previous_ppg_ts);
            double  raw_ref  = RPPGExtractor::process_signal(ppg_channel, ref);
            if (i_loop_ppg % UPDATE_RGB_GRAPH_STRIDE == 0) {
                graph_ppg->addData(current_ts_ppg, raw_ref);
                graph_ppg->data()->removeBefore(ts - win_length);
            }
        }
        previous_ppg_ts = ts;
        previous_ppg = ppg;
    }
}
void SignalProcess::reset_rppg() {
    current_ts = 0;
}

void SignalProcess::reset_ppg() {
    current_ts_ppg = 0;
    graph_ppg->data()->clear();
}
