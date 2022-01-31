#bin/bash 

# record a X second video on rv1126, then pull the file and play it back with gstreamer

rm -rf mamamia.h264

adb shell rm -rf /tmp/mamamia.h264

adb shell /oem/RkLunch-stop.sh

adb shell ./oem/usr/bin/consti_stream -a -w 1280 -h 720 -d rkispp_scale0 --ip_address 192.168.0.13 -b 5 -o /tmp/mamamia.h264 -t 5

echo "Done recording\n"


adb pull /tmp/mamamia.h264


gst-launch-1.0 filesrc location=mamamia.h264 ! h264parse ! avdec_h264 ! xvimagesink

