#include "imageviewer.h"
#include<iostream>
#include <QMouseEvent>
#include <qdebug>
#include <QGridLayout>
ImageViewer::ImageViewer(QWidget* parent) : QWidget(parent)
{
   save_parent = parent;
}

void ImageViewer::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    if (!m_img.isNull()) {
        setAttribute(Qt::WA_OpaquePaintEvent);
        p.drawImage(0, 0, m_img);
        painted = true;
    }

    if (isSelecting) {
        p.drawRect(QRect(startPoint, endPoint));
    }
}
void ImageViewer::mouseMoveEvent(QMouseEvent* event)
{
    if (left_key_pressed) {
        isSelecting = true;
        endPoint = event->pos();
        update(); 
    }
}

void ImageViewer::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        startPoint = event->pos();
        pressTime.start();
        left_key_pressed = true;
    }
    else if (event->button() == Qt::RightButton) {
        event_lock.lock();
        click_point = event->pos();
        click_event_type = -1;
        event_lock.unlock();
    }
}
void ImageViewer::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && left_key_pressed) {
        endPoint = event->pos();
        left_key_pressed = false;
        isSelecting = false;

        int elapsedTime = pressTime.elapsed(); 
        int distance = (endPoint - startPoint).manhattanLength();

        if (elapsedTime < timeThreshold && distance < moveThreshold) {
            event_lock.lock();
            click_point = startPoint;
            click_event_type = 1;
            event_lock.unlock();
        }
        else {
            event_lock.lock();
            click_event_type = 2;
            click_point = startPoint;
            click_point_end = endPoint;
            event_lock.unlock();
        }
        update();  // ×îÖÕ»æÖÆ
    }

}

void ImageViewer::setImage(const QImage &img)
{ 
    if (m_img.size() == img.size() && m_img.format() == img.format()
        && m_img.bytesPerLine() == img.bytesPerLine())
        std::copy_n(img.bits(), img.sizeInBytes(), m_img.bits());
    else
        m_img = img.copy();
    painted = false;
    update();
}