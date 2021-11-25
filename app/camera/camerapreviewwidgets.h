#ifndef CAMERAPREVIEWWIDGETS_H
#define CAMERAPREVIEWWIDGETS_H


#include "basewidget.h"
#include "cameraquickcontentwidget.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QQuickWidget>
#include <QMediaPlayer>


class cameraPreviewwidgets:public BaseWidget
{
    Q_OBJECT
public:
    cameraPreviewwidgets(QWidget *parent);
    QMediaPlayer* getMediaPlayerFormQml(){return m_player;}
    cameraQuickContentWidget* getContentWidget(){return m_contentWid;}
private:
    QVBoxLayout *vmainlyout;
    // mediaplayer类， 将传递给视频主界面videoWidgets做控制
    QMediaPlayer *m_player;

    cameraQuickContentWidget *m_contentWid;

    void initLayout();
};

#endif
