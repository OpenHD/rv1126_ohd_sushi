#
# Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

# vi: set noexpandtab syntax=cmake:

aux_source_directory(utilFtp FTP_UTIL_SRC)
aux_source_directory(utilList LIST_UTIL_SRC)
aux_source_directory(${PROJECT_SOURCE_DIR}/common ISP_COMMON_SRC)

if(ROCKX)
if(V4L2_CAPTURE)
#--------------------------
#  rkmedia_face_capture_test
#--------------------------
link_directories(${PROJECT_SOURCE_DIR}/librtsp/)
add_executable(rkmedia_face_capture_test rkmedia_face_capture_test.c ${PROJECT_SOURCE_DIR}/common/sample_common_isp.c ${FTP_UTIL_SRC})
add_dependencies(rkmedia_face_capture_test easymedia)
target_link_libraries(rkmedia_face_capture_test rtsp rockx easymedia rga)
target_include_directories(rkmedia_face_capture_test PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SYSROOT}/usr/include/rockx ${PROJECT_SOURCE_DIR})
install(TARGETS rkmedia_face_capture_test RUNTIME DESTINATION "bin")
endif() #if(V4L2_CAPTURE)
endif() #if(ROCKX)

if(ROCKX)
if(V4L2_CAPTURE)
#--------------------------
#  rkmedia_rockx_person_detect_test
#--------------------------
link_directories(${PROJECT_SOURCE_DIR}/librtsp/)
add_executable(rkmedia_rockx_person_detect_test rkmedia_rockx_person_detect_test.c ${PROJECT_SOURCE_DIR}/common/sample_common_isp.c ${PROJECT_SOURCE_DIR}/rockx_demo/utilList/rkmedia_list_method.c)
add_dependencies(rkmedia_rockx_person_detect_test easymedia)
target_link_libraries(rkmedia_rockx_person_detect_test rtsp rockx easymedia rga)
target_include_directories(rkmedia_rockx_person_detect_test PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SYSROOT}/usr/include/rockx ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/rockx_demo)
install(TARGETS rkmedia_rockx_person_detect_test RUNTIME DESTINATION "bin")
endif() #if(V4L2_CAPTURE)
endif() #if(ROCKX)