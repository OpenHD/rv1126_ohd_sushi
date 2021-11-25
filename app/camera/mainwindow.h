#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedLayout>
#include <QStackedWidget>
#include <camerawidgets.h>
#include <base/basewindow.h>
#include <QThread>
#include <ueventthread.h>

class MainWindow : public BaseWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    QStackedLayout *m_mainlyout;
    QStackedWidget *m_stackedWid;
    cameraWidgets *m_cameraWid;
private:
    UeventThread *ueventThread;
protected:
    void keyPressEvent(QKeyEvent *event);
    // Used for disable or enable application when car-reverse event comes.
    void disableApplication();
    void enableApplication();
private:
    void initData();
    void initLayout();
private slots:
    void slot_appQuit();
    void slot_standby();
public slots:
    void slot_returnanimation();
    void slot_uevent(const char *action, const char *dev);
};



#endif // MAINWINDOW_H
