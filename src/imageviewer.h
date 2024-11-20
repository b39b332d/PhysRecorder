#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <opencv2/core/types.hpp>
#include <mutex>
#include <QVBoxLayout>

class ImageViewer : public QWidget
{
    Q_OBJECT

    bool painted = true;
    QImage m_img;
    void paintEvent(QPaintEvent*);


    QWidget* save_parent;
    Qt::WindowFlags save_WindowFlags;
    QSize save_pSize;
public:
    std::mutex event_lock;
    QPoint click_point;
    int click_event_type;
    QVBoxLayout* save_layout;
    explicit ImageViewer(QWidget* parent = nullptr);
    Q_SLOT void setImage(const QImage&);
protected:
    void mousePressEvent(QMouseEvent* event);
};

#endif // IMAGEVIEWER_H
