#pragma once

#include <QLineEdit>
class SilentLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    SilentLineEdit(QWidget *parent = nullptr);

protected:
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

signals:
    void leftClicked(); // Custom signal to emit on left click
    void rightClicked(); // Custom signal to emit on left click

};
