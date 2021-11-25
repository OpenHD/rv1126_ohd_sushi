#ifndef BASEWINDOW_H
#define BASEWINDOW_H

#include <QObject>
#include <QPaintEvent>
#include <QPainterPath>
#include <qmath.h>
#include <QPainter>
#include <QStyleOption>
#include <QGridLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QSocketNotifier>

#include "basewidget.h"
#include "absframelessautosize.h"

/**
 * Base window of the application and it enable to move、resize、
 * drag and some other operation.
 *
 * Note: this window provided a 'm_mainwid' to modify user's own ui.
 */
class BaseWindow : public AbsFrameLessAutoSize
{
    Q_OBJECT
public:
    explicit BaseWindow(QWidget *parent = 0);
    BaseWidget *m_mainwid;
private:
    QFile fileIn;

    bool m_drag;
    QPoint m_dragPosition;
protected:
    virtual void paintEvent(QPaintEvent *);

    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

    // Used for disable or enable application when car-reverse event comes.
    virtual void disableApplication(){}
    virtual void enableApplication(){}
private slots:
    /* Read information sended from parent process. */
    void slot_readFromServer(int);
};
#endif // BASEWINDOW_H
