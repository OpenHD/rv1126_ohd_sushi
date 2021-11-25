#include "basepushbutton.h"

FlatButton::FlatButton(QWidget *parent):QPushButton(parent)
  ,longPressedFlag(false)
{
    setCursor(Qt::PointingHandCursor);
    setFlat(true);
    setStyleSheet("QPushButton{background:transparent;}");

    m_timer = new QTimer(this);
    connect(m_timer,SIGNAL(timeout()),this,SLOT(slot_timerTimeout()));
}

void FlatButton::slot_timerTimeout()
{
    m_timer->stop();
    longPressedFlag = true;
    // Once longPressedFlag be setted,send signal every 500 milliseconds.
    m_timer->start(500);
    emit longPressedEvent();
}

void FlatButton::mousePressEvent(QMouseEvent *e)
{
    QPushButton::mousePressEvent(e);
    m_timer->start(1000);
}

void FlatButton::mouseReleaseEvent(QMouseEvent *e)
{
    if(longPressedFlag){
        e->accept();
    }else{
        QPushButton::mouseReleaseEvent(e);
    }
    m_timer->stop();
    longPressedFlag = false;
}

FlatButton::FlatButton(const QString& str, QWidget *parent):QPushButton(str,parent)
{
    setCursor(Qt::PointingHandCursor);
    setFlat(true);
    setStyleSheet("QPushButton{background:transparent;}");
}

VolButton::VolButton(const QString& normal,QWidget*parent):QPushButton(parent)//5个连一串
{
    m_partnerslider=NULL;
    m_isenter=false;
    m_index=0;
    m_isvolempty=100;
    m_savevol=100;
    setCursor(Qt::PointingHandCursor);

    QPixmap pix(normal);

    for(int i=0;i<5;i++)
        m_listnormal<<pix.copy(i*(pix.width()/5),0,pix.width()/5,pix.height());

}

void VolButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.drawPixmap((width()-m_listnormal.at(0).width())/2,(height()-m_listnormal.at(0).height())/2,m_listnormal.at(m_index));
}

void VolButton::mousePressEvent(QMouseEvent *e)
{
    if(e->button()==Qt::LeftButton)
    {
        QPushButton::mousePressEvent(e);
    }
}

void VolButton::mouseReleaseEvent(QMouseEvent *e)
{
    if(e->button()==Qt::LeftButton)
    {
        if(this->contentsRect().contains(mapFromGlobal(QCursor::pos())))
        {
            if(m_isvolempty==0)
            {
                emit setMute(m_savevol);
            }
            else
            {
                m_savevol=m_partnerslider->value();
                emit setMute(0);
            }
        }
        QPushButton::mouseReleaseEvent(e);
    }
}

void VolButton::setButtonPixmap(int value)
{
    m_isenter=false;
    if(value==0)
        m_index=4;
    if(value>2&&value<=30)
        m_index=1;
    if(value>30)
        m_index=2;
    update();
    m_isvolempty=value;
}

void VolButton::enterEvent(QEvent *)
{
    m_isenter=true;
    update();
}

void VolButton::leaveEvent(QEvent *)
{
    m_isenter=false;
    update();
}

StackButton::StackButton(const QString& pixnormal,const QString& pixhover,const QString& pixsel,QWidget*parent):QPushButton(parent)
{
    m_enter=false;
    m_pressed=false;
    m_pixnormal=QPixmap(pixnormal);
    m_pixhover=QPixmap(pixhover);
    m_pixselected=QPixmap(pixsel);
    setCursor(Qt::PointingHandCursor);
    setFlat(true);
}

void StackButton::paintEvent(QPaintEvent *e)
{
    QPushButton::paintEvent(e);
    QPainter p(this);
    if(!m_enter&&!m_pressed)
        p.drawPixmap((width()-m_pixnormal.width())/2,(height()-m_pixnormal.height())/2,m_pixnormal);

    if(m_enter&&!m_pressed)
        p.drawPixmap((width()-m_pixhover.width())/2,(height()-m_pixhover.height())/2,m_pixhover);

    if(m_pressed)
        p.drawPixmap((width()-m_pixselected.width())/2,(height()-m_pixselected.height())/2,m_pixselected);

}

void StackButton::setselected(bool sel)
{
    m_pressed=sel;
    update();
}

void StackButton::mousePressEvent(QMouseEvent *e)
{
    QPushButton::mousePressEvent(e);
    if(e->button()==Qt::LeftButton)
    {
        m_pressed=true;
        update();
    }
}

void StackButton::enterEvent(QEvent *e)
{
    QPushButton::enterEvent(e);
    m_enter=true;
    update();
}

void StackButton::leaveEvent(QEvent *e)
{
    QPushButton::leaveEvent(e);
    m_enter=false;
    update();
}

GuideButton::GuideButton(QString pixnormal, QString text, QWidget *parent):QPushButton(parent)
{
    this->setCursor(Qt::PointingHandCursor);
    setStyleSheet("border-image: url("+pixnormal+");");
    m_pix=QPixmap(pixnormal);
    m_text=text;
    m_enter = false;
}

void GuideButton::enterEvent(QEvent *)
{
    m_enter = true;
    update();
}

void GuideButton::leaveEvent(QEvent *)
{
    m_enter = false;
    update();
}

FourStateButton::FourStateButton(QString pixnormal,QWidget *parent):QPushButton(parent)
{
    this->setCursor(Qt::PointingHandCursor);
    m_index=0;
    m_enter=false;
    QPixmap pix(pixnormal);
    for(int i=0;i<4;i++)
        m_pixlist<<pix.copy(i*(pix.width()/4),0,pix.width()/4,pix.height());
}

void FourStateButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.drawPixmap((width()-m_pixlist.at(m_index).width())/2,(height()-m_pixlist.at(m_index).height())/2
                       ,m_pixlist.at(m_index).width()
                       ,m_pixlist.at(m_index).height(),m_pixlist.at(m_index));//画图画到中间
}

void FourStateButton::enterEvent(QEvent *)
{
    m_index=1;
    m_enter=true;
    update();
}

void FourStateButton::leaveEvent(QEvent *)
{
    m_index=0;
    m_enter=false;
    update();
}

void FourStateButton::mousePressEvent(QMouseEvent *e)
{
    if(e->button()==Qt::LeftButton)
    {
        m_index=2;
        update();
        QPushButton::mousePressEvent(e);
    }
}

void FourStateButton::mouseReleaseEvent(QMouseEvent *e)
{
    if(e->button()==Qt::LeftButton)
    {
        m_index=1;
        update();
        QPushButton::mouseReleaseEvent(e);
    }
}
