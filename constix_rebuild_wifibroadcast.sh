#!/bin/bash

WFB_BUILDROOT_DIR="buildroot/output/rockchip_rv1126_rv1109/build/wifibroadcast-1.0.0"
WFB_SOURCE_DIR="external/wifibroadcast"

rm -rf $WFB_BUILDROOT_DIR

echo "removed directory"

./build.sh 


adb push $WFB_BUILDROOT_DIR/wfb_tx /oem/usr/bin/wfb_tx
adb push $WFB_BUILDROOT_DIR/wfb_rx /oem/usr/bin/wfb_rx
adb push $WFB_BUILDROOT_DIR/udp_generator_validator /oem/usr/bin/udp_generator_validator
adb push $WFB_BUILDROOT_DIR/benchmark /oem/usr/bin/benchmark
adb push $WFB_BUILDROOT_DIR/unit_test /oem/usr/bin/unit_test
adb push $WFB_BUILDROOT_DIR/wfb_keygen /oem/usr/bin/wfb_keygen

adb push $WFB_SOURCE_DIR/rv1126/enable_monitor_mode.sh /oem/
adb push $WFB_SOURCE_DIR/rv1126/start_tx_simple.sh /oem/


echo "pushed executable"