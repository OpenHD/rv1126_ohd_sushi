#include "mainwindow.h"
#include "global_value.h"

#include <QDebug>

MainWindow::MainWindow(QWidget *parent):BaseWindow(parent)
{
    initData();
    initLayout();
    connect(m_cameraWid->m_topWid,SIGNAL(returnClick()),this,SLOT(slot_appQuit()));
}

MainWindow::~MainWindow()
{
}

void MainWindow::initData(){
    mainWindow = this;
    ueventThread = new UeventThread(this);
    ueventThread->start();
}

void MainWindow::initLayout(){
    QVBoxLayout *mainLayout = new QVBoxLayout;

    m_cameraWid = new cameraWidgets(this);
    mainLayout->addWidget(m_cameraWid);
    mainLayout->setContentsMargins(0,0,0,0);

    setLayout(mainLayout);
}

void MainWindow::slot_appQuit()
{
    this->close();
}

void MainWindow::slot_returnanimation()
{
    qDebug() << "closeCameraApp";
    m_cameraWid->closeCamera();
    this->close();
}

void MainWindow::disableApplication()
{
    qDebug("disable camera application.");
    this->setVisible(false);
}

void MainWindow::enableApplication()
{
    this->setVisible(true);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    qDebug()<<"Received ketpress event with key value:"<<event->key();
    switch(event->key()){
    case Qt::Key_PowerOff:
        QTimer::singleShot(100, this, SLOT(slot_standby()));
        break;
    default:
        break;
    }
}

void MainWindow::slot_standby()
{
}

void MainWindow::slot_uevent(const char *action, const char *dev)
{
    qDebug()<<action<<"======"<<dev;
    if(m_cameraWid){
        m_cameraWid->uevent(action, dev);
    }
}
