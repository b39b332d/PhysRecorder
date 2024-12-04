#ifndef CONVERTER_H
#define CONVERTER_H

#include <QObject>
#include <QImage>
#include <QTimer>
#include <qlabel.h>
#include <opencv2/imgproc.hpp>
#include <qevent.h>
#include "imageviewer.h"
#include <CameraDriver.h>
#include <frame_types.h>

class Converter : public QObject {
   Q_OBJECT
   ImageViewer* videoLabel;
   cv::Size2i dst_size;

   cv::Mat m_frame;  // pre allocate
   cv::Mat m_canves;

   QImage q_frame;

public:
   Converter(ImageViewer* videoLabel,QObject * parent = nullptr);
   void frame_ready(QList<RawFrame*> main_frames, QList<RawFrame*> other_frames, cv::Rect2i face);
	   
   Q_SIGNAL void frameReady(const QImage&);
   Q_SIGNAL void device_selected(capture::CameraDevice*);
   Q_SIGNAL void set_roi(cv::Rect2i);

};
#endif // CONVERTER_H
