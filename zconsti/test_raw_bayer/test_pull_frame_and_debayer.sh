#bin/bash 

# capture a raw camera frame, pull it and debayer it locally

rm -rf mamamia.raw

adb shell rm -rf /tmp/mamamia.raw

adb shell ./oem/usr/bin/v4l2_test_mplane /dev/video0 1 /tmp/mamamia.raw

adb pull /tmp/mamamia.raw

bayer2rgb --input=mamamia.raw --output=mamamia.raw.tiff --width=1920 --height=1080 --bpp=16 --first=GBRG --method=VNG --tiff --swap
