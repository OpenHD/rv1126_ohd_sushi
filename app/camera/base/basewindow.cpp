#include "basewindow.h"
#include <QMessageBox>

#define MSG_DISABLE_APPLICATION "APP_DISABLE"
#define MSG_ENABLE_APPLICATION "APP_ENABLE"

BaseWindow::BaseWindow(QWidget *parent):AbsFrameLessAutoSize(parent)
  , m_drag(false)
{
    setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    // The main widget in this window.
    // Just modify the 'm_mainwid' if you wan't to modify the UI.
    m_mainwid = new BaseWidget(this);
    m_mainwid->setAutoFillBackground(true);

    /* Accoding to the message from parent process for next action */
    fileIn.open(stdin, QIODevice::ReadOnly);
    QSocketNotifier* sn = new QSocketNotifier(fileIn.handle(), QSocketNotifier::Read, this);
    connect(sn, SIGNAL(activated(int)), this, SLOT(slot_readFromServer(int)));
}

void BaseWindow::slot_readFromServer(int fd)
{
    if(fd != fileIn.handle())
        return;

    if(strncmp(fileIn.readLine().data(),MSG_DISABLE_APPLICATION,strlen(MSG_DISABLE_APPLICATION)) == 0){
        disableApplication();
    }else{
        enableApplication();
    }
}

void BaseWindow::paintEvent(QPaintEvent *e)
{
    AbsFrameLessAutoSize::paintEvent(e);
    QPainterPath path;
    path.setFillRule(Qt::WindingFill);
    path.addRect(9, 9, this->width()-18, this->height()-18);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillPath(path, QBrush(Qt::white));


    QColor color(0, 0, 0, 50);
    for(int i=0; i<9; i++)
    {
        QPainterPath path;
        path.setFillRule(Qt::WindingFill);
        path.addRect(9-i, 9-i, this->width()-(9-i)*2, this->height()-(9-i)*2);
        color.setAlpha(150 - qSqrt(i)*50);
        painter.setPen(color);
        painter.drawPath(path);
    }
}

void BaseWindow::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton /*&& rectMove.contains(event->pos())*/) {
        m_drag = true;
        m_dragPosition = event->globalPos() - this->pos();
    }
    QWidget::mousePressEvent(event);
}

void BaseWindow::mouseReleaseEvent(QMouseEvent *event)
{
    m_drag = false;
    QWidget::mouseReleaseEvent(event);
}

void BaseWindow::mouseMoveEvent(QMouseEvent *event)
{
    if(m_drag) {
        move(event->globalPos() - m_dragPosition);
    }
    QWidget::mouseMoveEvent(event);
}
