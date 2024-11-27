#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <opencv2/core/types.hpp>
#include <mutex>
#include <QVBoxLayout>
#include <QElapsedTimer>

class ImageViewer : public QWidget
{
    Q_OBJECT

    bool painted = true;
    QImage m_img;
    void paintEvent(QPaintEvent*);


    QWidget* save_parent;
    Qt::WindowFlags save_WindowFlags;
    QSize save_pSize;


    bool isSelecting = false;
    bool left_key_pressed = false;
    QPoint startPoint;
    QRect select_rect;
    QPoint endPoint;
    QElapsedTimer pressTime;
    const int timeThreshold = 300;
    const int moveThreshold = 5;

public:
    std::mutex event_lock;
    QPoint click_point;
    QPoint click_point_end;
    int click_event_type;
    QVBoxLayout* save_layout;
    explicit ImageViewer(QWidget* parent = nullptr);
    Q_SLOT void setImage(const QImage&);
protected:
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
};

#endif // IMAGEVIEWER_H
