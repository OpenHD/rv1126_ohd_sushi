#ifndef CAMERATOPWIDGETS_H
#define CAMERATOPWIDGETS_H
#include <QLabel>
#include "basewidget.h"
#include "basepushbutton.h"

class CameraTopWidgets:public BaseWidget
{
    Q_OBJECT
public:
    CameraTopWidgets(QWidget *parent=0);
    ~CameraTopWidgets(){}

    FourStateButton *m_btnreturn;
    void setTitle(const QString& title){titleLabel->setText(title);}
private:
    QLabel *titleLabel;

    void initLayout();
    void initConnection();
signals:
    void returnClick();
};

#endif // CAMERATOPWIDGETS_H
