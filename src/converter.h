#ifndef CONVERTER_H
#define CONVERTER_H

#include <QObject>
#include <QImage>
#include <QTimer>
#include <qlabel.h>
#include <opencv2/imgproc.hpp>
#include <qevent.h>
#include "imageviewer.h"

class Converter : public QObject {
   Q_OBJECT
   QImage m_image;
   cv::Size2i dst_size;
   cv::Size2i cur_size;
   cv::Size2i scaled_size;
   cv::Point2i offset;
   QTimer* refresh_time;
   cv::Mat previous_frame;
   ImageViewer* videoLabel;
   double scale, info_rec_scale;
   cv::Mat m_frame;  // pre allocate
   cv::Mat m_canves;
   cv::Mat info_rec, m_info_rec, Maxed_Canves;
   QImage q_frame;
   QPainter painter;
   cv::Scalar_<int> text_color;
   bool is_maxed=false;
   bool is_peak = false;
   float hr, br, fps, e_Happy, e_Neutral, e_Sad, e_Suprise, e_Anger,age;
   bool gender;

public:
   Converter(ImageViewer* videoLabel,QObject * parent = nullptr);
   Q_SLOT void processFaces(const cv::Mat &frame, std::vector<cv::Rect>*);
   Q_SLOT void processFace(const cv::Mat& frame, cv::Rect);
   Q_SLOT void updateFrame(const cv::Mat& frame);
   Q_SLOT void refresh();
   Q_SLOT void start();
   Q_SIGNAL void frameReady(QImage&);


   std::atomic<bool> isMouseEventProcessed; //lock
   cv::Point2i mouse;
   bool mouseCancel;

private :
	void rounded_rectangle(cv::Mat&, cv::Point, cv::Point, cv::Scalar, int, int, int);
	void generate_m_frame(const cv::Mat&);
};
#endif // CONVERTER_H
