#!/bin/bash 



rm -rf buildroot/output/rockchip_rv1126_rv1109/build/linux_custom

echo "removed buildroot/output/rockchip_rv1126_rv1109/build/linux_custom directory"

./build.sh kernel

./build.sh

echo "build kernel, update.img is ready for flashin"


