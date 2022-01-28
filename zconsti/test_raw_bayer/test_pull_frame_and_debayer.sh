#bin/bash 

# capture a raw camera frame, pull it and debayer it locally

rm -rf mamamia.raw

adb shell rm -rf /tmp/mamamia.raw

# disable packed
adb shell "echo 0 > /sys/devices/platform/rkcif_mipi_lvds/compact_test"
adb shell "cat /sys/devices/platform/rkcif_mipi_lvds/compact_test"

#adb shell "media-ctl -v -d /dev/media0 --set-v4l2 '"m00_f_imx415 1-001a":0[fmt:SGBRG10_1X10/1920x1080]'"

adb shell ./oem/usr/bin/v4l2_test_mplane -i /dev/video0 -o /tmp/mamamia.raw -n 1 -f 1

# re-enable packed
adb shell "echo 1 > /sys/devices/platform/rkcif_mipi_lvds/compact_test"
adb shell "cat /sys/devices/platform/rkcif_mipi_lvds/compact_test"


adb pull /tmp/mamamia.raw

bayer2rgb --input=mamamia.raw --output=mamamia.raw.tiff --width=1920 --height=1080 --bpp=16 --first=GBRG --method=VNG --tiff --swap

display mamamia.raw.tiff