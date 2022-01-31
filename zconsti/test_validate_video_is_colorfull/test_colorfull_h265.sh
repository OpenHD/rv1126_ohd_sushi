#bin/bash 

# record a X second video on rv1126, then pull the file and play it back with gstreamer

rm -rf mamamia.h265

adb shell rm -rf /tmp/mamamia.h265

adb shell /oem/RkLunch-stop.sh

adb shell ./oem/usr/bin/consti_stream -a -w 1280 -h 720 -d rkispp_scale0 --ip_address 192.168.0.13 -b 5 -o /tmp/mamamia.h265 -t 60 -e 1

echo "Done recording\n"


adb pull /tmp/mamamia.h265


gst-launch-1.0 filesrc location=mamamia.h265 ! h265parse ! avdec_h265 ! xvimagesink

