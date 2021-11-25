#include "cameratopwidgets.h"
#include <QHBoxLayout>
#include "global_value.h"

CameraTopWidgets::CameraTopWidgets(QWidget *parent):BaseWidget(parent)
{
    // Set background color.
    setObjectName("CameraTopWidgets");
    setStyleSheet("#CameraTopWidgets{background:rgb(56,58,66)}");

    initLayout();
    initConnection();
}

void CameraTopWidgets::initLayout()
{
    setWindowOpacity(0.1);
    setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);

    QHBoxLayout *hmainyout=new QHBoxLayout;

    m_btnreturn=new FourStateButton(return_resource_str,this);
    m_btnreturn->setFixedSize(return_icon_width,return_icon_height);

    QHBoxLayout *lyout1 = new QHBoxLayout;
    lyout1->addWidget(m_btnreturn);
    lyout1->addStretch(0);
    lyout1->setContentsMargins(0,0,0,0);

    titleLabel = new QLabel(this);
    QFont font = titleLabel->font();
    font.setPixelSize(font_size_big);
    titleLabel->setFont(font);
    titleLabel->setAlignment(Qt::AlignCenter);

    hmainyout->addLayout(lyout1,1);
    hmainyout->addWidget(titleLabel,1);
    hmainyout->addStretch(1);
    hmainyout->setContentsMargins(0,0,0,0);
    hmainyout->setSpacing(0);
    setLayout(hmainyout);

    titleLabel->setText(tr("camera"));
}

void CameraTopWidgets::initConnection()
{
    connect(m_btnreturn,SIGNAL(clicked(bool)),this,SIGNAL(returnClick()));
}
