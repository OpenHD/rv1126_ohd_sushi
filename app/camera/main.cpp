#include "mainwindow.h"
#include <QApplication>
#include <QTranslator>
#include "translations/language.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator translator;
    bool load=translator.load(Language::instance()->getCurrentQM());
    qApp->installTranslator(&translator);

    MainWindow w;
    w.showFullScreen();

    return a.exec();
}
