#include "imageviewer.h"
#include<iostream>
#include <QMouseEvent>
#include <qdebug>
#include <QGridLayout>
ImageViewer::ImageViewer(QWidget* parent) : QWidget(parent)
{
   save_parent = parent;
   is_maximized = false;
}

void ImageViewer::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    if (!m_img.isNull()) {
        setAttribute(Qt::WA_OpaquePaintEvent);
        p.drawImage(0, 0, m_img);
        painted = true;
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