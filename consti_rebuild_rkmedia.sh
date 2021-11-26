#!/bin/bash

# to rebuild anything under the "external" directory, remove its folder 
# and then run ./build.sh
# buildroot will detect that, sync its sources from source and rebuild only the removed folder



rm -rf buildroot/output/rockchip_rv1126_rv1109/build/rkmedia

echo "removed directory"

./build.sh 

echo "rebuild done"

adb push buildroot/output/rockchip_rv1126_rv1109/build/rkmedia/examples/consti_stream /oem/usr/bin/consti_stream
adb push buildroot/output/rockchip_rv1126_rv1109/build/rkmedia/examples/consti_run_isp /oem/usr/bin/consti_run_isp
adb push buildroot/output/rockchip_rv1126_rv1109/build/rkmedia/examples/consti_latency /oem/usr/bin/consti_latency

echo "flashing libs"
adb push buildroot/output/rockchip_rv1126_rv1109/build/rkmedia/src/libeasymedia.so        /oem/usr/lib/libeasymedia.so
adb push buildroot/output/rockchip_rv1126_rv1109/build/rkmedia/src/libeasymedia.so.1      /oem/usr/lib/libeasymedia.so.1
adb push buildroot/output/rockchip_rv1126_rv1109/build/rkmedia/src/libeasymedia.so.1.0.1  /oem/usr/lib/libeasymedia.so.1.0.1


echo "flashing done"