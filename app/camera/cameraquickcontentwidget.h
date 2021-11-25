#ifndef CAMERAQUICKCONTENTWIDGET_H
#define CAMERAQUICKCONTENTWIDGET_H

#include <QQuickWidget>
#include <QObject>
#include <QTimer>


class cameraQuickContentWidget:public QQuickWidget
{
    Q_OBJECT
public:
    cameraQuickContentWidget(QWidget *parent = 0);

private:
    void init();
};
#endif
