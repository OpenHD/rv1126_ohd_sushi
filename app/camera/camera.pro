#-------------------------------------------------
#
# Project created by QtCreator 2017-06-30T08:50:55
#
#-------------------------------------------------

QT  += core gui quickwidgets multimedia multimediawidgets multimedia-private
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = cameraView
TEMPLATE = app

# 3399Linux、Big DPI
DEFINES += DEVICE_EVB

CONFIG += link_pkgconfig
GST_VERSION = 1.0

PKGCONFIG += \
    gstreamer-$$GST_VERSION \
    gstreamer-base-$$GST_VERSION \
    gstreamer-audio-$$GST_VERSION \
    gstreamer-video-$$GST_VERSION \
    gstreamer-pbutils-$$GST_VERSION

lessThan(QT_MINOR_VERSION, 12) {
    LIBS += -lqgsttools_p
} else {
    LIBS += -lQt5MultimediaGstTools
}


DEFINES += HAVE_GST_PHOTOGRAPHY
LIBS += -lgstphotography-$$GST_VERSION
DEFINES += GST_USE_UNSTABLE_API #prevents warnings because of unstable photography API

DEFINES += HAVE_GST_ENCODING_PROFILES

DEFINES += USE_V4L

INCLUDEPATH +=$$PWD base
include(base/base.pri)

SOURCES += main.cpp\
        mainwindow.cpp \
        cameratopwidgets.cpp \
        camerawidgets.cpp \
        cameraquickcontentwidget.cpp \
        camerapreviewwidgets.cpp \
        global_value.cpp \
        ueventthread.cpp \
    translations/language.cpp

HEADERS += mainwindow.h \
        cameratopwidgets.h \
        camerawidgets.h \
        cameraquickcontentwidget.h \
        camerapreviewwidgets.h \
        global_value.h \
        ueventthread.h \
    translations/language.h

RESOURCES += \
    res_main.qrc \
    i18n.qrc \
    i18n.qrc
