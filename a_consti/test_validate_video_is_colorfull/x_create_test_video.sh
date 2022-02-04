#bin/bash 

adb shell rm -rf /tmp/rv1126_test.h264

adb shell ./oem/usr/bin/mpi_enc_test -w 1280 -h 720 -o /tmp/rv1126_test.h264 -n 100

adb pull /tmp/rv1126_test.h264

gst-launch-1.0 filesrc location=rv1126_test.h264 ! h264parse ! avdec_h264 ! xvimagesink

