#
# Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

# vi: set noexpandtab syntax=cmake:

project(rkmedia_examples)

add_definitions(-g -O0 -ggdb -gdwarf -funwind-tables -rdynamic)
# Consti10
add_definitions(-Wno-unused-function)
add_definitions(-Wno-unused-variable)
add_definitions(-Wno-unused-label)
add_definitions(-Wno-maybe-uninitialized)
add_definitions(-Wno-unused-parameter)

aux_source_directory(common COMMON_SRC)
aux_source_directory(common_consti COMMON_CONSTI_SRC)

############################################################
# RKMPP Module tests
############################################################
if(RKMPP)
if(RKMPP_ENCODER)
if(V4L2_CAPTURE)
#--------------------------
# rkmedia_vi_venc_test
#--------------------------
add_executable(rkmedia_vi_venc_test rkmedia_vi_venc_test.c ${COMMON_SRC})
add_dependencies(rkmedia_vi_venc_test easymedia)
target_link_libraries(rkmedia_vi_venc_test easymedia)
target_include_directories(rkmedia_vi_venc_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_vi_venc_test RUNTIME DESTINATION "bin")

#Consti10: Here I add the executable that is based on rkmedia_vi_venc_test but can be used for live streaming
# and might differ otherwise (I use it for testing)
add_executable(consti_stream consti_stream.cpp ${COMMON_SRC} ${COMMON_CONSTI_SRC})
add_dependencies(consti_stream easymedia)
target_link_libraries(consti_stream easymedia)
target_include_directories(consti_stream PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS consti_stream RUNTIME DESTINATION "bin")
#Consti10
add_executable(consti_run_isp consti_run_isp.c ${COMMON_SRC})
add_dependencies(consti_run_isp easymedia)
target_link_libraries(consti_run_isp easymedia)
target_include_directories(consti_run_isp PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS consti_run_isp RUNTIME DESTINATION "bin")
#Consti10
add_executable(consti_latency consti_latency.cpp ${COMMON_SRC} ${COMMON_CONSTI_SRC})
add_dependencies(consti_latency easymedia)
target_link_libraries(consti_latency easymedia)
target_include_directories(consti_latency PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS consti_latency RUNTIME DESTINATION "bin")

#--------------------------
# rkmedia_vi_venc_rtsp_test
#--------------------------
link_directories(${PROJECT_SOURCE_DIR}/librtsp/)
add_executable(rkmedia_vi_venc_rtsp_test rkmedia_vi_venc_rtsp_test.c ${COMMON_SRC})
add_dependencies(rkmedia_vi_venc_rtsp_test easymedia)
target_link_libraries(rkmedia_vi_venc_rtsp_test easymedia rtsp)
target_include_directories(rkmedia_vi_venc_rtsp_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_vi_venc_rtsp_test RUNTIME DESTINATION "bin")

#--------------------------
# rkmedia_venc_rotation_test
#--------------------------
link_directories(${PROJECT_SOURCE_DIR}/librtsp/)
add_executable(rkmedia_venc_rotation_test rkmedia_venc_rotation_test.c ${COMMON_SRC})
add_dependencies(rkmedia_venc_rotation_test easymedia)
target_link_libraries(rkmedia_venc_rotation_test easymedia rtsp)
target_include_directories(rkmedia_venc_rotation_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_venc_rotation_test RUNTIME DESTINATION "bin")

#--------------------------
# rkmedia_venc_osd_test
#--------------------------
add_executable(rkmedia_venc_osd_test rkmedia_venc_osd_test.c ${COMMON_SRC})
add_dependencies(rkmedia_venc_osd_test easymedia)
target_link_libraries(rkmedia_venc_osd_test easymedia)
target_include_directories(rkmedia_venc_osd_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_venc_osd_test RUNTIME DESTINATION "bin")

#--------------------------
#  rkmedia_venc_jpeg_test
#--------------------------
add_executable(rkmedia_venc_jpeg_test rkmedia_venc_jpeg_test.c ${COMMON_SRC})
add_dependencies(rkmedia_venc_jpeg_test easymedia)
target_link_libraries(rkmedia_venc_jpeg_test easymedia)
target_include_directories(rkmedia_venc_jpeg_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_venc_jpeg_test RUNTIME DESTINATION "bin")

#--------------------------
#  rkmedia_venc_mjpeg_test
#--------------------------
add_executable(rkmedia_venc_mjpeg_test rkmedia_venc_mjpeg_test.c ${COMMON_SRC})
add_dependencies(rkmedia_venc_mjpeg_test easymedia)
target_link_libraries(rkmedia_venc_mjpeg_test easymedia)
target_include_directories(rkmedia_venc_mjpeg_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_venc_mjpeg_test RUNTIME DESTINATION "bin")

#--------------------------
#  rkmedia_venc_cover_test
#--------------------------
add_executable(rkmedia_venc_cover_test rkmedia_venc_cover_test.c ${COMMON_SRC})
add_dependencies(rkmedia_venc_cover_test easymedia)
target_link_libraries(rkmedia_venc_cover_test easymedia)
target_include_directories(rkmedia_venc_cover_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_venc_cover_test RUNTIME DESTINATION "bin")


#--------------------------
#  rkmedia_venc_smartp_test
#--------------------------
add_executable(rkmedia_venc_smartp_test rkmedia_venc_smartp_test.c ${COMMON_SRC})
add_dependencies(rkmedia_venc_smartp_test easymedia)
target_link_libraries(rkmedia_venc_smartp_test easymedia)
target_include_directories(rkmedia_venc_smartp_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_venc_smartp_test RUNTIME DESTINATION "bin")

#--------------------------
#  rkmedia_venc_avbr_test
#--------------------------
add_executable(rkmedia_venc_avbr_test rkmedia_venc_avbr_test.c ${COMMON_SRC})
add_dependencies(rkmedia_venc_avbr_test easymedia)
target_link_libraries(rkmedia_venc_avbr_test easymedia)
target_include_directories(rkmedia_venc_avbr_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_venc_avbr_test RUNTIME DESTINATION "bin")

#--------------------------
#  rkmedia_venc_roi_osd_test
#--------------------------
add_executable(rkmedia_venc_roi_osd_test rkmedia_venc_roi_osd_test.c ${COMMON_SRC})
add_dependencies(rkmedia_venc_roi_osd_test easymedia)
target_link_libraries(rkmedia_venc_roi_osd_test easymedia)
target_include_directories(rkmedia_venc_roi_osd_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_venc_roi_osd_test RUNTIME DESTINATION "bin")

#--------------------------
#  rkmedia_main_stream_with_jpeg_test
#--------------------------
add_executable(rkmedia_main_stream_with_jpeg_test rkmedia_main_stream_with_jpeg_test.c ${COMMON_SRC})
add_dependencies(rkmedia_main_stream_with_jpeg_test easymedia)
target_link_libraries(rkmedia_main_stream_with_jpeg_test easymedia)
target_include_directories(rkmedia_main_stream_with_jpeg_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_main_stream_with_jpeg_test RUNTIME DESTINATION "bin")
endif() #if(V4L2_CAPTURE)

#--------------------------
#  rkmedia_venc_local_file_test
#--------------------------
add_executable(rkmedia_venc_local_file_test rkmedia_venc_local_file_test.c)
add_dependencies(rkmedia_venc_local_file_test easymedia)
target_link_libraries(rkmedia_venc_local_file_test easymedia)
target_include_directories(rkmedia_venc_local_file_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_venc_local_file_test RUNTIME DESTINATION "bin")
endif() #if(RKMPP_ENCODER)

if(RKMPP_DECODER)
if(DRM_DISPLAY)
#--------------------------
#  rkmedia_vdec_test
#--------------------------
add_executable(rkmedia_vdec_test rkmedia_vdec_test.c ${COMMON_SRC})
add_dependencies(rkmedia_vdec_test easymedia)
target_link_libraries(rkmedia_vdec_test easymedia)
target_include_directories(rkmedia_vdec_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_vdec_test RUNTIME DESTINATION "bin")
endif() #if(DRM_DISPLAY)
endif() #if(RKMPP_DECODER)
endif() #if(RKMPP)


############################################################
# ISP Module tests
############################################################
if(USE_RKAIQ)
if(RKMPP)
if(V4L2_CAPTURE)
#--------------------------
#  rkmedia_isp_test
#--------------------------
add_executable(rkmedia_isp_test rkmedia_isp_test.c ${COMMON_SRC})
add_dependencies(rkmedia_isp_test easymedia)
target_link_libraries(rkmedia_isp_test easymedia)
target_include_directories(rkmedia_isp_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_isp_test RUNTIME DESTINATION "bin")

#--------------------------
#  rkmedia_fake_vi_test
#--------------------------
add_executable(rkmedia_fake_vi_test rkmedia_fake_vi_test.c ${COMMON_SRC})
add_dependencies(rkmedia_fake_vi_test easymedia)
target_link_libraries(rkmedia_fake_vi_test easymedia)
target_include_directories(rkmedia_fake_vi_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_fake_vi_test RUNTIME DESTINATION "bin")

#--------------------------
#  rkmedia_snapshot_test
#--------------------------
add_executable(rkmedia_snapshot_test rkmedia_snapshot_test.c ${COMMON_SRC})
add_dependencies(rkmedia_snapshot_test easymedia)
target_link_libraries(rkmedia_snapshot_test easymedia)
target_include_directories(rkmedia_snapshot_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_snapshot_test RUNTIME DESTINATION "bin")

endif() #if(V4L2_CAPTURE)
endif() #if(RKMPP)
endif() #if(USE_RKAIQ)

############################################################
# AUDIO Module tests
############################################################
if(ALSA_CAPTURE)
#--------------------------
#  rkmedia_ai_test
#--------------------------
add_executable(rkmedia_ai_test rkmedia_ai_test.c)
add_dependencies(rkmedia_ai_test easymedia)
target_link_libraries(rkmedia_ai_test easymedia)
target_include_directories(rkmedia_ai_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_ai_test RUNTIME DESTINATION "bin")

#--------------------------
#  rkmedia_multi_ai_test
#--------------------------
add_executable(rkmedia_multi_ai_test multi_audio_test/rkmedia_multi_ai_test.c)
add_dependencies(rkmedia_multi_ai_test easymedia)
target_link_libraries(rkmedia_multi_ai_test easymedia)
target_include_directories(rkmedia_multi_ai_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_multi_ai_test RUNTIME DESTINATION "bin")

if(AUDIO_ENCODER)
#--------------------------
#  rkmedia_aenc_test
#--------------------------
add_executable(rkmedia_aenc_test rkmedia_aenc_test.c)
add_dependencies(rkmedia_aenc_test easymedia)
target_link_libraries(rkmedia_aenc_test easymedia)
target_include_directories(rkmedia_aenc_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_aenc_test RUNTIME DESTINATION "bin")

#--------------------------
#  rkmedia_ai_aenc_test
#--------------------------
add_executable(rkmedia_ai_aenc_test rkmedia_ai_aenc_test.c)
add_dependencies(rkmedia_ai_aenc_test easymedia)
target_link_libraries(rkmedia_ai_aenc_test easymedia)
target_include_directories(rkmedia_ai_aenc_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_ai_aenc_test RUNTIME DESTINATION "bin")
endif() #if(AUDIO_ENCODER)
endif() #if(ALSA_CAPTURE)

if(ALSA_PLAYBACK)
#--------------------------
#  rkmedia_ao_test
#--------------------------
add_executable(rkmedia_ao_test rkmedia_ao_test.c)
add_dependencies(rkmedia_ao_test easymedia)
target_link_libraries(rkmedia_ao_test easymedia)
target_include_directories(rkmedia_ao_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_ao_test RUNTIME DESTINATION "bin")

#--------------------------
#  rkmedia_multi_ao_test
#--------------------------
add_executable(rkmedia_multi_ao_test multi_audio_test/rkmedia_multi_ao_test.c)
add_dependencies(rkmedia_multi_ao_test easymedia)
target_link_libraries(rkmedia_multi_ao_test easymedia)
target_include_directories(rkmedia_multi_ao_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_multi_ao_test RUNTIME DESTINATION "bin")

if(AUDIO_DECODER)
#--------------------------
#  rkmedia_adec_ao_test
#--------------------------
add_executable(rkmedia_adec_ao_test rkmedia_adec_ao_test.c)
add_dependencies(rkmedia_adec_ao_test easymedia)
target_link_libraries(rkmedia_adec_ao_test easymedia)
target_include_directories(rkmedia_adec_ao_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_adec_ao_test RUNTIME DESTINATION "bin")
endif() #if(AUDIO_DECODER)
endif() #if(ALSA_PLAYBACK)

if(ALSA_CAPTURE AND ALSA_PLAYBACK AND AUDIO_ENCODER AND AUDIO_DECODER AND AUDIO_ALGORITHM)
#--------------------------
#  rkmedia_audio_test
#--------------------------
add_executable(rkmedia_audio_test rkmedia_audio_test.c)
add_dependencies(rkmedia_audio_test easymedia)
target_link_libraries(rkmedia_audio_test easymedia)
target_include_directories(rkmedia_audio_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_audio_test RUNTIME DESTINATION "bin")
endif()

############################################################
# MD Module tests
############################################################
if(MOVE_DETECTION)
#--------------------------
#  rkmedia_vi_md_test
#--------------------------
add_executable(rkmedia_vi_md_test rkmedia_vi_md_test.c ${COMMON_SRC})
add_dependencies(rkmedia_vi_md_test easymedia)
target_link_libraries(rkmedia_vi_md_test easymedia)
target_include_directories(rkmedia_vi_md_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_vi_md_test RUNTIME DESTINATION "bin")
endif() #if(MOVE_DETECTION)


############################################################
# OD Module tests
############################################################
if(OCCLUSION_DETECTION)
#--------------------------
#  rkmedia_vi_od_test
#--------------------------
add_executable(rkmedia_vi_od_test rkmedia_vi_od_test.c ${COMMON_SRC})
add_dependencies(rkmedia_vi_od_test easymedia)
target_link_libraries(rkmedia_vi_od_test easymedia)
target_include_directories(rkmedia_vi_od_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_vi_od_test RUNTIME DESTINATION "bin")
endif() #if(OCCLUSION_DETECTION)


############################################################
# RGA Module tests
############################################################
if(RKRGA)
if(V4L2_CAPTURE)
#--------------------------
# rkmedia_rga_api_test
#--------------------------
add_executable(rkmedia_rga_api_test rkmedia_rga_api_test.c ${COMMON_SRC})
add_dependencies(rkmedia_rga_api_test easymedia)
target_link_libraries(rkmedia_rga_api_test easymedia rga)
target_include_directories(rkmedia_rga_api_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_rga_api_test RUNTIME DESTINATION "bin")

#--------------------------
#  rkmedia_vi_rga_test
#--------------------------
add_executable(rkmedia_vi_rga_test rkmedia_vi_rga_test.c ${COMMON_SRC})
add_dependencies(rkmedia_vi_rga_test easymedia)
target_link_libraries(rkmedia_vi_rga_test easymedia)
target_include_directories(rkmedia_vi_rga_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_vi_rga_test RUNTIME DESTINATION "bin")

#--------------------------
#  rkmedia_rga_osd_test
#--------------------------
add_executable(rkmedia_rga_osd_test rkmedia_rga_osd_test.c ${COMMON_SRC})
add_dependencies(rkmedia_rga_osd_test easymedia)
target_link_libraries(rkmedia_rga_osd_test easymedia rga)
target_include_directories(rkmedia_rga_osd_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_rga_osd_test RUNTIME DESTINATION "bin")

#--------------------------
# rkmedia_rga_crop_venc_test
#--------------------------
link_directories(${PROJECT_SOURCE_DIR}/librtsp/)
add_executable(rkmedia_rga_crop_venc_test rkmedia_rga_crop_venc_test.c ${COMMON_SRC})
add_dependencies(rkmedia_rga_crop_venc_test easymedia)
target_link_libraries(rkmedia_rga_crop_venc_test rtsp easymedia rga)
target_include_directories(rkmedia_rga_crop_venc_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_rga_crop_venc_test RUNTIME DESTINATION "bin")
endif() #if(V4L2_CAPTURE)
endif() #if(RKRGA)


############################################################
# ROCKX Module tests
############################################################
if(ROCKX)
if(V4L2_CAPTURE)
#--------------------------
#  rkmedia_vi_rockx_venc_rtsp_test
#--------------------------
link_directories(${PROJECT_SOURCE_DIR}/librtsp/)
add_executable(rkmedia_vi_rockx_venc_rtsp_test rkmedia_vi_rockx_venc_rtsp_test.c ${COMMON_SRC})
add_dependencies(rkmedia_vi_rockx_venc_rtsp_test easymedia)
target_link_libraries(rkmedia_vi_rockx_venc_rtsp_test rtsp rockx easymedia rga)
target_include_directories(rkmedia_vi_rockx_venc_rtsp_test PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SYSROOT}/usr/include/rockx)
install(TARGETS rkmedia_vi_rockx_venc_rtsp_test RUNTIME DESTINATION "bin")
install(FILES rtsp-nn.cfg DESTINATION share)
endif() #if(V4L2_CAPTURE)
endif() #if(ROCKX)

############################################################
# RKNN Module tests
############################################################
if(RKNN)
if(V4L2_CAPTURE)
#--------------------------
#  rkmedia_vi_rknn_venc_rtsp_test
#--------------------------
link_directories(${PROJECT_SOURCE_DIR}/librtsp/)
add_executable(rkmedia_vi_rknn_venc_rtsp_test rkmedia_vi_rknn_venc_rtsp_test.c ${COMMON_SRC})
add_dependencies(rkmedia_vi_rknn_venc_rtsp_test easymedia)
target_link_libraries(rkmedia_vi_rknn_venc_rtsp_test rtsp rknn_api m easymedia rga)
target_include_directories(rkmedia_vi_rknn_venc_rtsp_test PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SYSROOT}/usr/include/rknn)
install(TARGETS rkmedia_vi_rknn_venc_rtsp_test RUNTIME DESTINATION "bin")
install(FILES rtsp-nn.cfg DESTINATION share)
install(DIRECTORY rknn_model DESTINATION share)
endif() #if(V4L2_CAPTURE)
endif() #if(RKNN)


############################################################
# VIDEO CAPUTRE Module tests
############################################################
if(V4L2_CAPTURE)
#--------------------------
#  rkmedia_vi_get_frame_test
#--------------------------
add_executable(rkmedia_vi_get_frame_test rkmedia_vi_get_frame_test.c ${COMMON_SRC})
add_dependencies(rkmedia_vi_get_frame_test easymedia)
target_link_libraries(rkmedia_vi_get_frame_test easymedia)
target_include_directories(rkmedia_vi_get_frame_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_vi_get_frame_test RUNTIME DESTINATION "bin")

#--------------------------
#  rkmedia_vi_electrostatic_protection
#--------------------------
add_executable(rkmedia_vi_electrostatic_protection rkmedia_vi_electrostatic_protection.c ${COMMON_SRC})
add_dependencies(rkmedia_vi_electrostatic_protection easymedia)
target_link_libraries(rkmedia_vi_electrostatic_protection easymedia)
target_include_directories(rkmedia_vi_electrostatic_protection PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_vi_electrostatic_protection RUNTIME DESTINATION "bin")

#--------------------------
#  rkmedia_vi_luma_only_mode_test
#--------------------------
add_executable(rkmedia_vi_luma_only_mode_test rkmedia_vi_luma_only_mode_test.c ${COMMON_SRC})
add_dependencies(rkmedia_vi_luma_only_mode_test easymedia)
target_link_libraries(rkmedia_vi_luma_only_mode_test easymedia)
target_include_directories(rkmedia_vi_luma_only_mode_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_vi_luma_only_mode_test RUNTIME DESTINATION "bin")

if(DRM_DISPLAY)
#--------------------------
#  rkmedia_vi_insert_user_picture_test
#--------------------------
add_executable(rkmedia_vi_insert_user_picture_test rkmedia_vi_insert_user_picture_test.c ${COMMON_SRC})
add_dependencies(rkmedia_vi_insert_user_picture_test easymedia)
target_link_libraries(rkmedia_vi_insert_user_picture_test easymedia)
target_include_directories(rkmedia_vi_insert_user_picture_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_vi_insert_user_picture_test RUNTIME DESTINATION "bin")

#--------------------------
#  rkmedia_vi_double_cameras_test
#--------------------------
add_executable(rkmedia_vi_double_cameras_test rkmedia_vi_double_cameras_test.c ${COMMON_SRC})
add_dependencies(rkmedia_vi_double_cameras_test easymedia)
target_link_libraries(rkmedia_vi_double_cameras_test easymedia)
target_include_directories(rkmedia_vi_double_cameras_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_vi_double_cameras_test RUNTIME DESTINATION "bin")

#--------------------------
#  rkmedia_vi_double_cameras_switch_test
#--------------------------
add_executable(rkmedia_vi_double_cameras_switch_test rkmedia_vi_double_cameras_switch_test.c ${COMMON_SRC})
add_dependencies(rkmedia_vi_double_cameras_switch_test easymedia)
target_link_libraries(rkmedia_vi_double_cameras_switch_test easymedia)
target_include_directories(rkmedia_vi_double_cameras_switch_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_vi_double_cameras_switch_test RUNTIME DESTINATION "bin")
endif() #if(DRM_DISPLAY)

if(COMPILES_UVC_EXAMPLE)
#--------------------------
#  rkmedia_vi_uvc_test
#--------------------------
add_executable(rkmedia_vi_uvc_test rkmedia_vi_uvc_test.c ${COMMON_SRC})
add_dependencies(rkmedia_vi_uvc_test easymedia)
target_link_libraries(rkmedia_vi_uvc_test easymedia rkuvc)
target_include_directories(rkmedia_vi_uvc_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_vi_uvc_test RUNTIME DESTINATION "bin")
endif() #if(COMPILES_UVC_EXAMPLE)

if(RKMPP_ENCODER)
#--------------------------
#  rkmedia_vi_multi_bind_test
#--------------------------
add_executable(rkmedia_vi_multi_bind_test rkmedia_vi_multi_bind_test.c ${COMMON_SRC})
add_dependencies(rkmedia_vi_multi_bind_test easymedia)
target_link_libraries(rkmedia_vi_multi_bind_test easymedia)
target_include_directories(rkmedia_vi_multi_bind_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_vi_multi_bind_test RUNTIME DESTINATION "bin")
endif() #if(RKMPP_ENCODER)
endif() #if(V4L2_CAPTURE)


############################################################
# DISPLAY Module tests
############################################################
if(DRM_DISPLAY)
if(V4L2_CAPTURE)
if(RKMEDIA_SOCKET)
#--------------------------
#  rkmedia_vo_sw_client_test
#--------------------------
add_executable(rkmedia_vo_sw_client_test rkmedia_vo_sw_client_test.c)
add_dependencies(rkmedia_vo_sw_client_test easymedia)
target_link_libraries(rkmedia_vo_sw_client_test easymedia_client)
target_include_directories(rkmedia_vo_sw_client_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_vo_sw_client_test RUNTIME DESTINATION "bin")
endif() #if(RKMEDIA_SOCKET)

#--------------------------
#  rkmedia_vi_vo_test
#--------------------------
add_executable(rkmedia_vi_vo_test rkmedia_vi_vo_test.c ${COMMON_SRC})
add_dependencies(rkmedia_vi_vo_test easymedia)
target_link_libraries(rkmedia_vi_vo_test easymedia)
target_include_directories(rkmedia_vi_vo_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_vi_vo_test RUNTIME DESTINATION "bin")

#--------------------------
#  rkmedia_vi_vp_vo_test
#--------------------------
add_executable(rkmedia_vi_vp_vo_test rkmedia_vi_vp_vo_test.c ${COMMON_SRC})
add_dependencies(rkmedia_vi_vp_vo_test easymedia)
target_link_libraries(rkmedia_vi_vp_vo_test easymedia)
target_include_directories(rkmedia_vi_vp_vo_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_vi_vp_vo_test RUNTIME DESTINATION "bin")

#--------------------------
#  rkmedia_vmix_vo_test
#--------------------------
add_executable(rkmedia_vmix_vo_test rkmedia_vmix_vo_test.c ${COMMON_SRC})
add_dependencies(rkmedia_vmix_vo_test easymedia)
target_link_libraries(rkmedia_vmix_vo_test easymedia)
target_include_directories(rkmedia_vmix_vo_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_vmix_vo_test RUNTIME DESTINATION "bin")

#--------------------------
#  rkmedia_vmix_vo_dvr_test
#--------------------------
add_executable(rkmedia_vmix_vo_dvr_test rkmedia_vmix_vo_dvr_test.c ${COMMON_SRC})
add_dependencies(rkmedia_vmix_vo_dvr_test easymedia)
target_link_libraries(rkmedia_vmix_vo_dvr_test easymedia)
target_include_directories(rkmedia_vmix_vo_dvr_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_vmix_vo_dvr_test RUNTIME DESTINATION "bin")

#--------------------------
#  rkmedia_vo_scale_test
#--------------------------
add_executable(rkmedia_vo_scale_test rkmedia_vo_scale_test.c ${COMMON_SRC})
add_dependencies(rkmedia_vo_scale_test easymedia)
target_link_libraries(rkmedia_vo_scale_test easymedia)
target_include_directories(rkmedia_vo_scale_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_vo_scale_test RUNTIME DESTINATION "bin")
endif() #if(V4L2_CAPTURE)

#--------------------------
#  rkmedia_vo_display_test
#--------------------------
add_executable(rkmedia_vo_display_test rkmedia_vo_display_test.c ${COMMON_SRC})
add_dependencies(rkmedia_vo_display_test easymedia)
target_link_libraries(rkmedia_vo_display_test easymedia)
target_include_directories(rkmedia_vo_display_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_vo_display_test RUNTIME DESTINATION "bin")
endif() #if(DRM_DISPLAY)

############################################################
# MUXER Module tests
############################################################
#--------------------------
# rkmedia_muxer_test
#--------------------------
add_executable(rkmedia_muxer_test rkmedia_muxer_test.c ${COMMON_SRC})
add_dependencies(rkmedia_muxer_test easymedia)
target_link_libraries(rkmedia_muxer_test easymedia)
target_include_directories(rkmedia_muxer_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkmedia_muxer_test RUNTIME DESTINATION "bin")

############################################################
# ROCKX TEST
############################################################
option(COMPILES_ROCKX_TEST "Enable rockx tests" ON)
if(COMPILES_ROCKX_TEST)
  add_subdirectory(rockx_demo)
endif()

############################################################
# STRESS TEST
############################################################
option(COMPILES_STRESS_TEST "Enable stress tests" OFF)
if(COMPILES_STRESS_TEST)
  add_subdirectory(stressTest)
endif()


############################################################
# UNINT TEST
############################################################
option(COMPILES_UNIT_TEST "Enable unit tests" OFF)
if(COMPILES_UNIT_TEST)
  add_subdirectory(unitTest)
endif()
