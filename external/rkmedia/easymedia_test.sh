#!/bin/bash
##
## Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
## Use of this source code is governed by a BSD-style license that can be
## found in the LICENSE file.
##

TOPDIR=$(cd $(dirname $(readlink -f $0));pwd)
CHIP=$1
PROJECT_TOPDIR=$(realpath $TOPDIR/../../)

if [ ! $CHIP ]; then
    echo missing chip platform
    exit -1
fi

TEMPDIR=$HOME/tmp/easymedia
rm -rf $TEMPDIR
mkdir -p $TEMPDIR

echo -e "output dir: "$TEMPDIR"\n"

adb push $PROJECT_TOPDIR/buildroot/output/rockchip_$CHIP/target/usr/lib/libeasymedia.so /usr/lib/

# rkmpp enc
adb push $PROJECT_TOPDIR/buildroot/output/rockchip_$CHIP/target/usr/bin/rkmpp_enc_test /usr/bin/
adb push ${TOPDIR}/rkmpp/test/mpp_enc_test_320_240.nv12 /tmp/

echo -e "1 #### rkmpp h264 encoder test\n"
adb shell rkmpp_enc_test -i /tmp/mpp_enc_test_320_240.nv12 -o /tmp/output.h264 -w 320 -h 240 -f nv12_h264
adb pull /tmp/output.h264 $TEMPDIR
adb shell rm /tmp/output.h264
echo -e "\n2 #### rkmpp mjpeg encoder test\n"
adb shell rkmpp_enc_test -i /tmp/mpp_enc_test_320_240.nv12 -o /tmp/output.mjpeg -w 320 -h 240 -f nv12_jpeg
adb pull /tmp/output.mjpeg $TEMPDIR
adb shell rm /tmp/output.mjpeg

# camera_cap_test -i /dev/video0 -o /tmp/output.yuv -w 1920 -h 1080 -f image:nv16