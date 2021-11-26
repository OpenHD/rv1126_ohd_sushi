#!/bin/bash

# after building the consti_test file under rkmedia (consti_rebuild_rkmedia.sh) use adb to push this file to the running dev board
# without re-flashing the whle firmware.

adb push buildroot/output/rockchip_rv1126_rv1109/build/rkmedia/examples/consti_test /oem/usr/bin/consti_test
