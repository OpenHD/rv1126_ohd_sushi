#!/bin/bash

rm -rf buildroot/output/rockchip_rv1126_rv1109/build/consti_demo-1.0.0

echo "removed directory"

./build.sh 


adb push buildroot/output/rockchip_rv1126_rv1109/build/consti_demo-1.0.0/consti_demo /oem/usr/bin/consti_demo
adb push buildroot/output/rockchip_rv1126_rv1109/build/consti_demo-1.0.0/freq /oem/usr/bin/freq

echo "pushed executable"