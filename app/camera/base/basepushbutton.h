#ifndef BASEPUSHBUTTON_H
#define BASEPUSHBUTTON_H
#include <QObject>
#include <QWidget>
#include <QPushButton>
#include <QEvent>
#include <QList>
#include <QMouseEvent>
#include <QTimer>
#include <QSlider>
#include <QPainter>
#include <QRect>

class FlatButton : public QPushButton
{
    Q_OBJECT
public:
    FlatButton(QWidget *parent=0);
    FlatButton(const QString& str,QWidget*parent=0);

    bool isLongPressed(){return longPressedFlag;}
private:
    // It stands for the button current is be long pressed.
    bool longPressedFlag;
    // Used for identify long press event.
    QTimer *m_timer;
private slots:
    void slot_timerTimeout();
protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
signals:
    void longPressedEvent();
};

class GuideButton : public QPushButton
{
    Q_OBJECT
public:
    explicit GuideButton(QString pixnormal,QString text,QWidget*parent);

protected:
    //void paintEvent(QPaintEvent *);
    void enterEvent(QEvent *);
    void leaveEvent(QEvent *);

private:
    QPixmap  m_pix;
    QString m_text;
    bool m_enter;
};

class FourStateButton:public QPushButton
{
    Q_OBJECT
public:
    FourStateButton(QString pix_listurl,QWidget*parent);
protected:
    void paintEvent(QPaintEvent *);
    void enterEvent(QEvent *);
    void leaveEvent(QEvent*);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e) override;
private:
    QList<QPixmap> m_pixlist;
    int m_index;
    bool m_enter;
};


class VolButton:public QPushButton
{
    Q_OBJECT
public:
    VolButton(const QString& normal,QWidget*parent=0);
    void setParentSlider(QSlider* slider){m_partnerslider=slider;}
protected:
    void enterEvent(QEvent*);
    void leaveEvent(QEvent*);
    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *e)override;
    void mouseReleaseEvent(QMouseEvent *e)override;
private:
    bool m_isenter;
    int m_savevol;
    int m_isvolempty;
    int m_index;
    QList<QPixmap> m_listnormal;
    QList<QPixmap> m_listhover;
    QList<QPixmap> m_listpressed;

    QSlider *m_partnerslider;
public slots:
    void setButtonPixmap(int);//getFromSlider
signals:
    void setMute(int);
};

class StackButton:public QPushButton
{
    Q_OBJECT
public:
    explicit StackButton(const QString& pixnormal,const QString& pixhover,const QString& pixsel,QWidget*parent);
    void setselected(bool=true);
protected:
    void mousePressEvent(QMouseEvent *e);
    void enterEvent(QEvent *);
    void leaveEvent(QEvent *);
    void paintEvent(QPaintEvent *);
private:
    int m_index;
    bool m_enter;
    bool m_pressed;
    QPixmap m_pixnormal;
    QPixmap m_pixhover;
    QPixmap m_pixselected;
};

#endif // BASEPUSHBUTTON_H
