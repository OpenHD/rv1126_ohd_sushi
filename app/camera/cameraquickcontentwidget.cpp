#include "cameraquickcontentwidget.h"

#include <QMouseEvent>
#include <QMimeData>
#include <QBitmap>
#include <QDesktopWidget>
#include <QApplication>

cameraQuickContentWidget::cameraQuickContentWidget(QWidget *parent):QQuickWidget(parent)
{
    init();
}

void cameraQuickContentWidget::init()
{
    this->setMouseTracking(true);
    this->setCursor(QCursor(Qt::ArrowCursor));
    this->setAutoFillBackground(true);
    setWindowOpacity(1);           //设置透明度
    this->setWindowFlags(Qt::FramelessWindowHint|Qt::WindowSystemMenuHint);
    this->setFocusPolicy(Qt::ClickFocus);
    this->setAcceptDrops(true);    //设置可以接受拖拽
}

