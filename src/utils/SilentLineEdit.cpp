#include "SilentLineEdit.h"
#include <QMouseEvent>

SilentLineEdit::SilentLineEdit(QWidget* parent) : QLineEdit(parent)
{
}

void SilentLineEdit::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton )
        emit leftClicked();
    else if(event->button() == Qt::RightButton)
        emit rightClicked();
    return;
}

void SilentLineEdit::contextMenuEvent(QContextMenuEvent* event)
{
    event->ignore();
}