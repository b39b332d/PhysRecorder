#include "converter.h"
#include<iostream>
#include<qdebug>
#include <qcursor.h>

#include <opencv2/highgui.hpp>
using namespace cv;
static int show_matirx[][2] = { {1,1},{1,1},{1,2},{2,2},{2,2},{2,3},{2,3},{3,3},{3,3},{3,3}
,{ 3,4 },{3,4},{3,4},{4,4},{4,4},{4,4},{4,4},{4,5},{4,5},{4,5},{4,5},{5,5},{5,5},{5,5},{5,5},{5,5}};
Converter::Converter(ImageViewer* videoLabel, QObject* parent) :
    QObject(parent),
    videoLabel(videoLabel)
{
}

inline void cvt_puttext(capture::CameraStream*stream, cv::Mat& m_frame, std::chrono::steady_clock::time_point& current_time) {
    if ((current_time - stream->previous_fps_time) > std::chrono::seconds(1)) {
        int elapsed = std::chrono::duration_cast<std::chrono::microseconds>((current_time - (stream->previous_fps_time))).count();
        stream->previous_fps = (float)((stream->count)) * 1e6 / elapsed;
        stream->previous_fps_time = current_time;
        stream->count = 0;
    }
    cv::putText(m_frame, std::format("{:.2f}fps",
        stream->previous_fps),
        { 30, 30 }, cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);

}


QSet<capture::CameraProfile*> previous_profiles;
QSet<capture::CameraProfile*> previous_main;
std::chrono::steady_clock::time_point previous_time = std::chrono::steady_clock::now();


#define Rawframe_cv_img(frame) *((cv::Mat*)(frame->bgr_frame))
void Converter::frame_ready(QList<RawFrame*> main_frames,QList<RawFrame*> other_frames,Rect2i face) {
    auto current_time = std::chrono::steady_clock::now();
    int total_width = videoLabel->width();
    int total_height = videoLabel->height();
    QSet<capture::CameraProfile*> current_profiles;
    QSet<capture::CameraProfile*> current_main;
    Point2i click_point, click_point_end;
    videoLabel->event_lock.lock();
    int click_event_type = videoLabel->click_event_type;
    if (click_event_type != 0) {
        click_point = Point2i(videoLabel->click_point.x(), videoLabel->click_point.y());
        click_point_end = Point2i(videoLabel->click_point_end.x(), videoLabel->click_point_end.y());
        videoLabel->click_event_type = 0;
    }
    videoLabel->event_lock.unlock();




    for (auto frame : main_frames) {
        current_profiles.insert((capture::CameraProfile*)(frame->profile));
        current_main.insert((capture::CameraProfile*)(frame->profile));
    }
    for (auto frame : other_frames) {
        current_profiles.insert((capture::CameraProfile*)(frame->profile));
    }
    QSet<capture::CameraProfile*> new_profiles = current_profiles - previous_profiles;
    QSet<capture::CameraProfile*> orig_profiles = current_profiles & previous_profiles;


    Size2i widget_size(total_width, total_height);
    if (dst_size != widget_size) {
        dst_size = widget_size;
        m_canves = Mat::zeros(dst_size, CV_8UC3);
    }
    if (current_profiles != previous_profiles || current_main != previous_main) {
        m_canves = Mat::zeros(dst_size, CV_8UC3);
    }
    if (main_frames.size() != 0) {
        int other_height = 100;
        int main_dst_height = total_height - other_height;
        int main_width = 0;
        int max_width = 0;
        for (auto pframe : main_frames) {
            main_width += (float)(RawFrame_WIDTH_(pframe)) * main_dst_height / RawFrame_HEIGHT_(pframe);
        }
        int dst_height; Point2i start_point;
        if (main_width <= total_width) {
            dst_height = main_dst_height;
            start_point = Point2i((total_width - main_width) / 2, 0);
        }
        else {
            dst_height = (float)main_dst_height / main_width * total_width;
            start_point = Point2i(0, (main_dst_height - dst_height) / 2);
        }            
        for (auto pframe : main_frames) {
            float scaled = (float)dst_height / RawFrame_HEIGHT_(pframe);
            Size2i scaled_size(scaled * RawFrame_WIDTH_(pframe), dst_height);
            Rect2i start_pt(start_point, scaled_size);
            m_frame = Mat(m_canves, start_pt);
            if (pframe->bgr_frame != nullptr) {
                cv::resize(Rawframe_cv_img(pframe),
                    m_frame, scaled_size, 0, 0, INTER_NEAREST);
            }
            else if (new_profiles.contains(RawFrame_PROFILE_(pframe))) {
                m_frame.setTo(cv::Scalar::all(0));
                int baseline = 0;
                cv::Size textSize = cv::getTextSize("no signal", cv::FONT_HERSHEY_SIMPLEX, 1.0, 1, &baseline);
                cv::Point textOrg((m_frame.cols - textSize.width) / 2, (m_frame.rows + textSize.height) / 2);
                cv::putText(m_frame, "no signal", textOrg, cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 255, 255), 1);
            }            
            if (face.area() != 0) {
                rectangle(m_frame, Rect(face.tl() * scaled, face.br() * scaled), Scalar_<int>(255, 0, 0));
            }
            cvt_puttext(RawFrame_PROFILE_(pframe)->stream, m_frame, current_time);
            if (click_event_type == -1)
                emit stream_selected(nullptr);
            else if (click_event_type == 2) {
                if (start_pt.contains(click_point) && start_pt.contains(click_point_end)) {
                    auto p1 = (click_point - start_point) / scaled;
                    auto p2 = (click_point_end - start_point) / scaled;
                    emit set_roi(cv::Rect2i(p1,p2));
                }
            }
            else if (click_event_type == 1) {
                emit set_roi({0,0,0,0});
            }
            pframe->release();
            start_point.x += scaled_size.width;
        }

        start_point = Point2i (0, main_dst_height);
        for (auto pframe : other_frames) {

            if (start_point.x < total_width) {
                float scaled = (float)other_height / RawFrame_HEIGHT_(pframe);
                Size2i scaled_size(scaled * RawFrame_WIDTH_(pframe), other_height);
                if (pframe->bgr_frame != nullptr) {
                    int residual_width = total_width - start_point.x;
                    if (residual_width >= scaled_size.width) {
                        Rect2i start_pt(start_point, scaled_size);
                        m_frame = Mat(m_canves, start_pt);
                        cv::resize(Rawframe_cv_img( pframe),
                            m_frame, scaled_size, 0, 0, INTER_NEAREST);
                        cvt_puttext(RawFrame_PROFILE_(pframe)->stream, m_frame, current_time);

                        if (click_event_type == 1 && start_pt.contains(click_point))
                            emit stream_selected(RawFrame_PROFILE_(pframe)->stream);
                        else if (click_event_type == -1)
                            emit stream_selected(nullptr);
                    }
                    else {
                        Rect2i start_pt(start_point, Size2i( residual_width ,scaled_size.height ));
                        m_frame = Mat(m_canves, start_pt);
                        cv::resize(cv::Mat(RawFrame_HEIGHT_(pframe),
                            residual_width / scaled,
                            CV_8UC3, pframe->bgr_frame, RawFrame_WIDTH_(pframe)*3),
                            m_frame, m_frame.size(), 0, 0, INTER_NEAREST);

                        if (click_event_type == 1 && start_pt.contains(click_point))
                            emit stream_selected(RawFrame_PROFILE_(pframe)->stream);
                        else if (click_event_type == -1)
                            emit stream_selected(nullptr);
                    }
                }
                start_point.x += scaled_size.width;
            }
            pframe->release();
        }
    }
    else {
        int n_hori, n_vert,n_total= other_frames.size();
        if (total_width < total_height) {
            n_hori = show_matirx[n_total][0];
            n_vert = show_matirx[n_total][1];
        }
        else {
            n_hori = show_matirx[n_total][1];
            n_vert = show_matirx[n_total][0];
        }
        int hori_size = total_width / n_hori;
        int vert_size = total_height / n_vert;
        int hori_n = 0, vert_n = 0;
        for (auto frame : other_frames) {
            float scaleh = (float)hori_size / RawFrame_WIDTH_(frame);
            float scalev = (float)vert_size / RawFrame_HEIGHT_(frame);
            float scale_dst;
            Rect2i start_pt;
            if (scalev > scaleh) {
                scale_dst = scaleh;
                int frame_v_size = scaleh * RawFrame_HEIGHT_(frame);
                int height_ofs = (vert_size - frame_v_size) / 2;
                start_pt = Rect2i(hori_n * hori_size, vert_n * vert_size+ height_ofs, hori_size, frame_v_size);
            }
            else {
                scale_dst = scalev;
                int frame_h_size = scalev * RawFrame_WIDTH_(frame);
                int width_ofs = (hori_size - frame_h_size) / 2;
                start_pt = Rect2i(hori_n * hori_size+ width_ofs, vert_n * vert_size, frame_h_size, vert_size);
            }
            m_frame = Mat(m_canves, start_pt);


            if (frame->bgr_frame != nullptr)
                cv::resize(Rawframe_cv_img(frame),
                    m_frame, m_frame.size(),0,0, INTER_NEAREST);
            cvt_puttext(RawFrame_PROFILE_(frame)->stream, m_frame, current_time);

            if (click_event_type == 1 && start_pt.contains(click_point))
                emit stream_selected(RawFrame_PROFILE_(frame)->stream);
            else if (click_event_type == 2) {
                if (start_pt.contains(click_point) && start_pt.contains(click_point_end)) {
                    auto p1 = (click_point - start_pt.tl()) / scale_dst;
                    auto p2 = (click_point_end - start_pt.tl()) / scale_dst;
                    emit stream_selected(RawFrame_PROFILE_(frame)->stream);
                    emit set_roi(cv::Rect2i(p1, p2));
                }
            }
            frame->release();

            hori_n++;
            if (hori_n == n_hori) {
                vert_n++;
                hori_n = 0;
            }
        }
    }
    previous_main = current_main;
    previous_profiles = current_profiles;
    previous_time = current_time;
    q_frame = QImage(m_canves.data, m_canves.cols, m_canves.rows, m_canves.step, QImage::Format_RGB888).rgbSwapped();
    emit frameReady(q_frame);
}
