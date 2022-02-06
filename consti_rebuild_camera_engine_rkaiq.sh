#!/bin/bash

# to rebuild anything under the "external" directory, remove its folder 
# and then run ./build.sh
# buildroot will detect that, sync its sources from source and rebuild only the removed folder



rm -rf buildroot/output/rockchip_rv1126_rv1109/build/camera_engine_rkaiq-1.0

echo "removed directory"

./build.sh 

echo "rebuild done"

adb push buildroot/output/rockchip_rv1126_rv1109/build/camera_engine_rkaiq-1.0/all_lib/Release/librkaiq.so /oem/usr/lib/librkaiq.so

#note: Not pushing the stuff under /oem/usr/include/rkaiq

adb push buildroot/output/rockchip_rv1126_rv1109/target/usr/bin/rkisp_demo /usr/bin/rkisp_demo
adb push buildroot/output/rockchip_rv1126_rv1109/target/usr/bin/rkisp_consti_demo /usr/bin/rkisp_consti_demo

echo "flashing done"