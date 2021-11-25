#ifndef UEVENTTHREAD_H
#define UEVENTTHREAD_H

#include <QThread>

class UeventThread:public QThread
{
public:
    UeventThread(QObject *parent=0);
    ~UeventThread();
protected:
    void run();
};

#endif // UEVENTTHREAD_H
