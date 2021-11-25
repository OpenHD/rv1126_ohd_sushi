#ifndef GLOBAL_VALUE_H
#define GLOBAL_VALUE_H

#include <QStandardPaths>
#include <mainwindow.h>

extern MainWindow *mainWindow;

#ifdef DEVICE_EVB
const int font_size = 32;
const int font_size_big = 40;
const int font_size_large = 60;
// top part
const QString return_resource_str = ":/image/main/return_big.png";
const int return_icon_width = 212;
const int return_icon_height = 70;
const int top_icon_size = 60;
#else
const int font_size = 13;
const int font_size_big = 18;
const int font_size_large = 35;
// top part
const QString return_resource_str = ":/image/main/return.png";
const int return_icon_width = 115;
const int return_icon_height = 40;
const int top_icon_size = 40;
#endif

#endif
