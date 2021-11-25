#ifndef LANGUAGE_H
#define LANGUAGE_H

#include <QObject>
#include <QSettings>
class Language : public QObject
{
    Q_OBJECT
private:
    explicit Language(QObject *parent = 0);
    static Language *s_instance;
    QSettings *i18nSetting;
public:
     static Language*  instance(){
         if(s_instance==0){
            s_instance=new Language();
         }
         return s_instance;
     }

     QStringList findQmFiles();
     bool languageMatch(const QString& lang, const QString& qmFile);

     void setLang(QString lang);
     QString getLang();
     QString getCurrentQM();


signals:

public slots:
};

#endif // LANGUAGE_H
