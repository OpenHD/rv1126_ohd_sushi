#!/bin/bash

FLASH_TOOL_BIN= ./tools/linux/Linux_Upgrade_Tool/Linux_Upgrade_Tool/upgrade_tool

#IMAGE_DIR= /media/consti10/INTENSO/a_newjear/35_aybering_dtsi_x6/update.img
IMAGE_DIR=IMAGE/RV1126-OPENHD_20220116.1702_RELEASE_TEST/IMAGES/update.img


$FLASH_TOOL_BIN uf $IMAGE_DIR