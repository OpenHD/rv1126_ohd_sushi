#include "basewidget.h"
#include <QStyleOption>
#include <QPainter>

BaseWidget::BaseWidget(QWidget *parent) : QWidget(parent)
{
    setMouseTracking(true);
}

void BaseWidget::paintEvent(QPaintEvent *)
{
    /* Slove the problem which 'setStyleSheet' and 'Q_OBJECT' can co-exist
       The below code used to repaint widgets when change became. */
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void BaseWidget::mousePressEvent(QMouseEvent *e)
{
    QWidget::mousePressEvent(e);
}

void BaseWidget::mouseMoveEvent(QMouseEvent *e)
{
    QWidget::mouseMoveEvent(e);
}

void BaseWidget::mouseReleaseEvent(QMouseEvent *e)
{
    QWidget::mouseReleaseEvent(e);
}
