#
# Copyright 2020 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

if(V4L2_CAPTURE)
if(RKMPP_ENCODER)
#--------------------------
#  rkmedia_venc_stress_test
#--------------------------
add_executable(rkmedia_venc_stress_test rkmedia_venc_stress_test.c)
add_dependencies(rkmedia_venc_stress_test easymedia)
target_link_libraries(rkmedia_venc_stress_test easymedia)
target_include_directories(rkmedia_venc_stress_test PRIVATE
                           ${CMAKE_SOURCE_DIR}/include
                           ${PROJECT_SOURCE_DIR})
install(TARGETS rkmedia_venc_stress_test RUNTIME DESTINATION "bin")

#--------------------------
#  rkmedia_vi_venc_change_resolution_test
#--------------------------
link_directories(${PROJECT_SOURCE_DIR}/librtsp/)
add_executable(rkmedia_vi_venc_change_resolution_test rkmedia_vi_venc_change_resolution_test.c
                                                      ${PROJECT_SOURCE_DIR}/common/sample_common_isp.c)
add_dependencies(rkmedia_vi_venc_change_resolution_test easymedia)
target_link_libraries(rkmedia_vi_venc_change_resolution_test rtsp easymedia)
target_include_directories(rkmedia_vi_venc_change_resolution_test PRIVATE
                           ${CMAKE_SOURCE_DIR}/include
                           ${PROJECT_SOURCE_DIR})
install(TARGETS rkmedia_vi_venc_change_resolution_test RUNTIME DESTINATION "bin")

#--------------------------
#  rkmedia_stream_stress_test
#--------------------------
add_executable(rkmedia_stream_stress_test rkmedia_stream_stress_test.c
                                          ${PROJECT_SOURCE_DIR}/common/sample_common_isp.c)
add_dependencies(rkmedia_stream_stress_test easymedia)
target_link_libraries(rkmedia_stream_stress_test easymedia)
target_include_directories(rkmedia_stream_stress_test PRIVATE
                           ${CMAKE_SOURCE_DIR}/include
                           ${PROJECT_SOURCE_DIR})
install(TARGETS rkmedia_stream_stress_test RUNTIME DESTINATION "bin")
endif() #if(RKMPP_ENCODER)

if(USE_RKAIQ)
#--------------------------
# rkmedia_isp_stresss_test
#--------------------------
link_directories(${PROJECT_SOURCE_DIR}/librtsp/)
add_executable(rkmedia_isp_stresss_test rkmedia_isp_stresss_test.c
                                        ${PROJECT_SOURCE_DIR}/common/sample_common_isp.c)
add_dependencies(rkmedia_isp_stresss_test easymedia)
target_link_libraries(rkmedia_isp_stresss_test rtsp easymedia)
target_include_directories(rkmedia_isp_stresss_test PRIVATE
                           ${CMAKE_SOURCE_DIR}/include
                           ${PROJECT_SOURCE_DIR})
install(TARGETS rkmedia_isp_stresss_test RUNTIME DESTINATION "bin")
endif() #if(USE_RKAIQ)
endif() #if(V4L2_CAPTURE)
