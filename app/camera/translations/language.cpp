#include "language.h"
#include <QDir>
#include <QSettings>
#include <QLocale>
#include <QDebug>

Language* Language::s_instance =  0;


#define SETTING_LANG "LANG"
Language::Language(QObject *parent) : QObject(parent)
{
    QDir settingsDir("/data/");

    if(settingsDir.exists()){
        i18nSetting=new QSettings("/data/i18n.ini", QSettings::IniFormat);
    }else{
        i18nSetting=new QSettings("/etc/i18n.ini", QSettings::IniFormat);
    }
}

QStringList Language::findQmFiles()
{
    QDir dir(":/translations");
    QStringList fileNames = dir.entryList(QStringList("*.qm"), QDir::Files,
                                          QDir::Name);
    QMutableStringListIterator i(fileNames);
    while (i.hasNext()) {
        i.next();
        i.setValue(dir.filePath(i.value()));
    }
    return fileNames;
}

bool Language::languageMatch(const QString& lang, const QString& qmFile)
{
    //qmFile: i18n_xx.qm
    const QString prefix = "i18n_";
    const int langTokenLength = 2; /*FIXME: is checking two chars enough?*/
    return qmFile.midRef(qmFile.indexOf(prefix) + prefix.length(), langTokenLength) == lang.leftRef(langTokenLength);
}

QString Language::getCurrentQM(){

    QStringList qmFiles =  Language::instance()->findQmFiles();
    qDebug()<<"getLang:"<<getLang()<<"qmFiles.size:"<<qmFiles.size();
    for (int i = 0; i < qmFiles.size(); ++i) {
        qDebug()<<  getLang()<<":" << qmFiles[i];
        if (Language::instance()->languageMatch(getLang(), qmFiles[i])){
            return qmFiles[i];
        }
    }
    return qmFiles[0];
}

QString Language::getLang(){
    QVariant defaultLang(QLocale::system().name()/*"zh_CN.UTF-8"*/);
    QVariant i18n= i18nSetting->value(SETTING_LANG,defaultLang);
    return i18n.toString();
}
void Language::setLang(QString lang){
    QVariant langVariant(lang);
    i18nSetting->setValue(SETTING_LANG,langVariant);
}
