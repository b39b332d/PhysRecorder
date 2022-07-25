#include "converter.h"
#include<iostream>
#include<qdebug>
#include <qcursor.h>

#include <opencv2/highgui.hpp>
using namespace cv;
Converter::Converter(ImageViewer* videoLabel, QObject* parent) :
    QObject(parent),
    videoLabel(videoLabel),
    scale(0),
    mouse(NULL),
    isMouseEventProcessed(true)
{
}

void Converter::start() {
    refresh_time = new QTimer();
    refresh_time->stop();
    connect(refresh_time, &QTimer::timeout, this, &Converter::refresh);
}


void Converter::processFaces(const cv::Mat& frame, std::vector<cv::Rect>* faces)
{
    QPoint globalCursorPos = videoLabel->mapFromGlobal(QCursor::pos());
    Point2i cursorPos = Point2i(globalCursorPos.x(), globalCursorPos.y()) - offset;
    generate_m_frame(frame);
    for (cv::Rect& face : *faces) {
        Rect scaled_face = Rect(face.tl() * scale, face.br() * scale);
        if (scaled_face.contains(cursorPos))
            rectangle(m_frame, scaled_face, Scalar_<int>(0, 255, 0));
        else
            rectangle(m_frame, scaled_face, Scalar_<int>(0, 0, 255));
    }
    delete faces;
    q_frame = QImage(m_canves.data, m_canves.cols, m_canves.rows, m_canves.step, QImage::Format_RGB888).rgbSwapped();
    emit frameReady(q_frame);
}
void Converter::processFace(const cv::Mat& frame, cv::Rect face)
{
    generate_m_frame(frame);
    rectangle(m_frame, Rect(face.tl() * scale, face.br() * scale), Scalar_<int>(255, 0, 0));

    q_frame = QImage(m_canves.data, m_canves.cols, m_canves.rows, m_canves.step, QImage::Format_RGB888).rgbSwapped();
    emit frameReady(q_frame);
}

void Converter::updateFrame(const cv::Mat& frame) {
    generate_m_frame(frame);
    q_frame = QImage(m_canves.data, m_canves.cols, m_canves.rows, m_canves.step, QImage::Format_RGB888).rgbSwapped();
    emit frameReady(q_frame);
}

void Converter::refresh() {
    Size2i widget_size(videoLabel->width(), videoLabel->height());
    if (dst_size != widget_size) {
        updateFrame(previous_frame);
    }
}


void Converter::generate_m_frame(const cv::Mat& frame) {
    refresh_time->start(100);
    previous_frame = frame;
    bool changed = false;
    Size2i widget_size(videoLabel->width(), videoLabel->height());
    Size2i frame_size = frame.size();

    if (dst_size != widget_size || cur_size != frame_size) {
        changed = true;
        dst_size = widget_size;
        cur_size = frame_size;
        m_canves = Mat::zeros(dst_size, CV_8UC3);

        scale = ((double)dst_size.width) / cur_size.width;
        if (scale * cur_size.height > dst_size.height) {
            scale = ((double)dst_size.height) / cur_size.height;
            scaled_size.height = dst_size.height;
            scaled_size.width = scale * cur_size.width;
            offset.x = (dst_size.width - scaled_size.width) / 2;
            offset.y = 0;
        }
        else {
            scaled_size.width = dst_size.width;
            scaled_size.height = scale * cur_size.height;
            offset.y = (dst_size.height - scaled_size.height) / 2;
            offset.x = 0;
        }
        m_frame = Mat(m_canves, Rect2i(offset, scaled_size));
    }
    resize(frame, m_frame, scaled_size);
}
