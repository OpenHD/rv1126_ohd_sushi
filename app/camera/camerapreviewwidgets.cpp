#include "camerapreviewwidgets.h"
#include <QHBoxLayout>
#include <QTime>
#include <QQuickItem>

cameraPreviewwidgets::cameraPreviewwidgets(QWidget *parent):BaseWidget(parent)
{
    setObjectName("cameraPreviewwidgets");
    setStyleSheet("#cameraPreviewwidgets{background:rgb(10,10,10)}");
    initLayout();
}

void cameraPreviewwidgets::initLayout()
{
    vmainlyout = new QVBoxLayout;

    // 改用qml播放视频
    m_contentWid = new cameraQuickContentWidget(this);
    m_contentWid->setResizeMode(QQuickWidget::SizeRootObjectToView);
    // set tanslate update for px3se
    //#ifndef DEVICE_EVB
    m_contentWid->setSource(QUrl("qrc:/camera_px3se.qml"));
    m_contentWid->setClearColor(QColor(Qt::transparent));
    /*#else
    m_contentWid->setSource(QUrl("qrc:/camera.qml"));
    #endif
    */
    // 处理逻辑，将qml中的player转而用QMediaPlayer代替，便于用C++语言进行控制
    QObject* qmlMediaPlayer = m_contentWid->rootObject()->findChild<QObject*>("camera");
    m_player = qvariant_cast<QMediaPlayer *>(qmlMediaPlayer->property("mediaObject"));

    vmainlyout->addWidget(m_contentWid);
    vmainlyout->setContentsMargins(0,0,0,0);
    vmainlyout->setSpacing(0);

    setLayout(vmainlyout);
}


